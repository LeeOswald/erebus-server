#include <erebus/log2.hxx>
#include <erebus/system/thread.hxx>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

#include <boost/circular_buffer.hpp>

namespace Er::Log2
{

namespace
{

class AsyncLogger
    : public ILogger
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
        if (s_threadData.size() < m_instanceId + 1)
            s_threadData.resize(m_instanceId + 1);
        else
            s_threadData[m_instanceId] = {};
    }

    void write(Record::Ptr r) override
    {
        r->indent = s_threadData[m_instanceId].indent;

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
        m_queueEmpty.wait(l, stop, [this]() { return !m_queue.empty(); });
    }

    void addSink(std::string_view name, ISink::Ptr sink) override
    {
        std::unique_lock l(m_mutexSinks);
        m_sinks.insert({ std::string(name), sink });
    }

    void removeSink(std::string_view name) override
    {
        std::unique_lock l(m_mutexSinks);
        m_sinks.erase(std::string(name));
    }

    void indent() override
    {
        ++s_threadData[m_instanceId].indent;
    }

    void unindent() override
    {
        ErAssert(s_threadData[m_instanceId].indent > 0);
        --s_threadData[m_instanceId].indent;
    }

private:
    static std::size_t makeInstanceId() noexcept
    {
        return s_instances.fetch_add(1, std::memory_order_relaxed);
    }

    void run(std::stop_token stop) noexcept
    {
        try
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
        catch (...)
        {
        }
    }

    void sendToSinks(const std::vector<Record::Ptr>& records, std::stop_token stop)
    {
        std::unique_lock l(m_mutexSinks);

        for (auto record : records)
        {
            for (auto& sink : m_sinks)
            {
                if (!record)
                {
                    sink.second->flush();
                }
                else
                {
                    sink.second->write(record);
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

    static const std::size_t MaxQueueSize = 1024;
    static std::atomic<std::size_t> s_instances;
    static thread_local std::vector<ThreadData> s_threadData;
    
    std::size_t m_instanceId;
    std::mutex m_mutexQueue;
    std::condition_variable_any m_queueNotEmpty;
    std::condition_variable_any m_queueEmpty;
    boost::circular_buffer<Record::Ptr> m_queue;
    std::mutex m_mutexSinks;
    std::unordered_map<std::string, ISink::Ptr> m_sinks;
    std::jthread m_worker;
};

std::atomic<std::size_t> AsyncLogger::s_instances = 0;
thread_local std::vector<AsyncLogger::ThreadData> AsyncLogger::s_threadData;

} // namespace {}



} // namespace Er::Log2 {}