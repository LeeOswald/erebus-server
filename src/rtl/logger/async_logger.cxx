#include "logger_base.hxx"

#include <erebus/rtl/empty.hxx>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <variant>


namespace Er::Log
{

namespace
{


class AsyncLogger
    : public Private::LoggerBase<Empty>
{
    using Base = Private::LoggerBase<Empty>;

public:
    ~AsyncLogger() = default;

    AsyncLogger(std::string_view component, std::chrono::milliseconds threshold)
        : Base(component, makeTee(ThreadSafe::Yes))
        , m_worker([this](std::stop_token stop) { run(stop); })
    {
    }
    
    void doWrite(Record::Ptr r) override
    {
        bool thresholdReached = false;
        {
            std::unique_lock l(m_mutexQueue);
            m_queue.emplace(r);

            if (m_queue.size() == 1)
            {
                // remember the oldest record's timestamp
                m_last = std::chrono::steady_clock::now(); 
            }

            // don't send out records too often
            if (m_threshold != std::chrono::milliseconds{})
            {
                auto now = std::chrono::steady_clock::now();
                auto delta = now - m_last;
                if (std::chrono::duration_cast<std::chrono::milliseconds>(delta) >= m_threshold)
                    thresholdReached = true;
            }
        }

        if (thresholdReached)
            m_queueNotEmpty.notify_one();
    }

    void doWrite(AtomicRecord a) override
    {
        if (a.empty())
            return;

        {
            std::unique_lock l(m_mutexQueue);

            m_queue.emplace(a);
        }

        m_queueNotEmpty.notify_one();
    }
    
    void flush() override
    {
        {
            std::unique_lock l(m_mutexQueue);

            // issue an empty record to force flushing all sinks
            m_queue.emplace(Record::Ptr{});
        }

        m_queueNotEmpty.notify_one();

        // wait until really flushed
        {
            auto stop = m_worker.get_stop_token();
            
            std::unique_lock l(m_mutexQueue);
            m_queueEmpty.wait(l, stop, [this]() { return m_queue.empty(); });
        }
    }
    
private:
    using AnyRecord = std::variant<Record::Ptr, AtomicRecord>;
    using RecordQueue = std::queue<AnyRecord>;

    void run(std::stop_token stop)
    {
        System::CurrentThread::setName("Logger");

        do
        {
            RecordQueue q;

            {
                std::unique_lock lw(m_mutexQueue);
                
                if (m_queue.empty())
                    m_queueEmpty.notify_all();

                m_queueNotEmpty.wait(lw, stop, [this]() { return !m_queue.empty(); });

                q.swap(m_queue);

                // clean the oldest record's timestamp
                m_last = std::chrono::time_point<std::chrono::steady_clock>::min();
            }

            // queue is now unlocked
            if (!q.empty())
            {
                sendToSinks(q, stop);
            }

        } while (!stop.stop_requested());
    }

    void sendToSinks(RecordQueue& records, std::stop_token stop)
    {
        while (!records.empty())
        {
            auto any = records.front();
            records.pop();

            auto r = std::get_if<Record::Ptr>(&any);

            if (r)
            {
                auto record = *r;
                if (!record)
                    m_tee->flush(); // empty record means forced flush
                else
                    m_tee->write(record);
            }
            else
            {
                auto a = std::get_if<AtomicRecord>(&any);
                if (a)
                    m_tee->write(*a);
            }

            if (stop.stop_requested())
                break;
        }
    }
    
    std::chrono::milliseconds m_threshold;
    std::mutex m_mutexQueue;
    std::condition_variable_any m_queueNotEmpty;
    std::condition_variable_any m_queueEmpty;
    RecordQueue m_queue;
    std::chrono::time_point<std::chrono::steady_clock> m_last = std::chrono::time_point<std::chrono::steady_clock>::min();
    std::jthread m_worker;
};


} // namespace {}


ER_RTL_EXPORT ILogger::Ptr makeLogger(std::string_view component, std::chrono::milliseconds threshold)
{
    return std::make_shared<AsyncLogger>(component, threshold);
}

} // namespace Er::Log {}