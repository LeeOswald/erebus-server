#include <erebus/log.hxx>
#include <erebus/system/thread.hxx>

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include <boost/circular_buffer.hpp>

namespace Er::Log
{

namespace
{

class AsyncLogger
    : public ILog
    , public NonCopyable
{
public:
    ~AsyncLogger()
    {
        s_instances--;
    }

    AsyncLogger()
        : m_instanceId(makeInstanceId())
        , m_queue(MaxQueueSize)
        , m_worker([this](std::stop_token stop) { run(stop); })
    {
        *threadData(m_instanceId) = {};
    }

    void write(Record::Ptr r) override
    {
        if (r->level() < level())
            return;

        r->setIndent(threadData(m_instanceId)->indent);

        {
            std::unique_lock l(m_mutexQueue);
            m_queue.push_back(r);
        }

        m_queueNotEmpty.notify_one();
    }

    void flush() override
    {
        std::unique_lock l(m_mutexQueue);
        
        // issue an empty record to force flushing all sinks
        m_queue.push_back(Record::Ptr{});
        m_queueNotEmpty.notify_one();

        auto stop = m_worker.get_stop_token();
        m_queueEmpty.wait(l, stop, [this]() { return m_queue.empty(); });
    }

    void addSink(std::string_view name, ISink::Ptr sink) override
    {
        std::unique_lock l(m_mutexSinks);
        m_sinks.push_back({ name, sink });
    }

    void removeSink(std::string_view name) override
    {
        std::unique_lock l(m_mutexSinks);
        auto it = std::find_if(m_sinks.begin(), m_sinks.end(), [name](const SinkData& d) { return d.name == name; });
        if (it != m_sinks.end())
            m_sinks.erase(it);
    }

    void indent() override
    {
        auto td = threadData(m_instanceId);
        ++td->indent;
    }

    void unindent() override
    {
        auto td = threadData(m_instanceId);
        ErAssert(td->indent > 0);
        --td->indent;
    }

private:
    static std::size_t makeInstanceId() noexcept
    {
        return s_instances.fetch_add(1, std::memory_order_relaxed);
    }

    void run(std::stop_token stop)
    {
        System::CurrentThread::setName("Logger");

        std::vector<Record::Ptr> records;
        records.reserve(MaxQueueSize);

        while (!stop.stop_requested())
        {
            {
                std::unique_lock lw(m_mutexQueue);
                m_queueNotEmpty.wait(lw, stop, [this]() { return !m_queue.empty(); });

                while (!stop.stop_requested() && !m_queue.empty())
                {
                    auto record = m_queue.front();
                    records.push_back(record);
                    m_queue.pop_front();
                }
            }

            // queue is now unlocked
            if (!records.empty())
            {
                sendToSinks(records, stop);
                records.clear();
            }

            m_queueEmpty.notify_all();
        }
    }

    void sendToSinks(const std::vector<Record::Ptr>& records, std::stop_token stop)
    {
        std::unique_lock l(m_mutexSinks);

        for (auto record : records)
        {
            for (auto& sink : m_sinks)
            {
                if (sink.healthy)
                {
                    try
                    {
                        if (!record)
                        {
                            sink.sink->flush();
                        }
                        else
                        {
                            sink.sink->write(record);
                        }
                    }
                    catch (...)
                    {
                        sink.healthy = false;
                    }
                }
            }

            if (stop.stop_requested())
                break;
        }
    }

    struct ThreadData
    {
        unsigned indent = 0;

        ThreadData() noexcept = default;
    };

    struct SinkData
    {
        std::string name;
        ISink::Ptr sink;
        bool healthy = true;

        SinkData(std::string_view name, ISink::Ptr sink)
            : name(name)
            , sink(sink)
        {
        }
    };

    static ThreadData* threadData(std::size_t instanceId)
    {
        if (s_threadData.size() < instanceId + 1)
            s_threadData.resize(instanceId + 1);
        
        return &s_threadData[instanceId];
    }

    static const std::size_t MaxQueueSize = 1024;
    static std::atomic<std::size_t> s_instances;
    static thread_local std::vector<ThreadData> s_threadData;
    
    const std::size_t m_instanceId;
    std::mutex m_mutexQueue;
    std::condition_variable_any m_queueNotEmpty;
    std::condition_variable_any m_queueEmpty;
    boost::circular_buffer<Record::Ptr> m_queue;
    std::mutex m_mutexSinks;
    std::vector<SinkData> m_sinks;
    std::jthread m_worker;
};

std::atomic<std::size_t> AsyncLogger::s_instances = 0;
thread_local std::vector<AsyncLogger::ThreadData> AsyncLogger::s_threadData;

} // namespace {}


EREBUS_EXPORT ILog::Ptr makeAsyncLogger()
{
    return std::make_shared<AsyncLogger>();
}

} // namespace Er::Log {}