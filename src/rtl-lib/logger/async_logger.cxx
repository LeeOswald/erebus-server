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

    AsyncLogger(std::string_view component, std::chrono::milliseconds threshold, std::int32_t maxQueueSize)
        : Base(component, makeTee(ThreadSafe::Yes))
        , m_threshold(threshold)
        , MaxQueueSize(maxQueueSize)
        , m_worker([this](std::stop_token stop) { run(stop); })
    {
    }
    
    void doWrite(RecordPtr r) override
    {
        std::int32_t discarded = 0;
        bool thresholdReached = false;
        {
            std::unique_lock l(m_mutexWQueue);

            // avoid queue overflow
            while (m_wQueue.size() + 1 > MaxQueueSize)
            {
                m_wQueue.pop();
                ++discarded;
            }

            m_wQueue.push(r);

            if (m_wQueue.size() == 1)
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

        {
            std::unique_lock l(m_mutexRQueue);
            ++m_pendingRecords;
            m_pendingRecords -= discarded;
        }

        if (thresholdReached)
            m_wQueueNotEmpty.notify_one();
    }

    void doWrite(AtomicRecordPtr a) override
    {
        std::int32_t discarded = 0;
        {
            std::unique_lock l(m_mutexWQueue);

            // avoid queue overflow
            while (m_wQueue.size() + 1 > MaxQueueSize)
            {
                ++discarded;
                m_wQueue.pop();
            }

            m_wQueue.push(a);
        }

        {
            std::unique_lock l(m_mutexRQueue);
            ++m_pendingRecords;
            m_pendingRecords -= discarded;
        }

        // ignore m_threshold when we see an atomic record
        m_wQueueNotEmpty.notify_one();
    }
    
    bool flush(std::chrono::milliseconds timeout) override
    {
        std::int32_t discarded = 0;
        {
            std::unique_lock l(m_mutexWQueue);

            // avoid queue overflow
            while (m_wQueue.size() + 1 > MaxQueueSize)
            {
                ++discarded;
                m_wQueue.pop();
            }

            // issue an empty record to force flushing all sinks
            m_wQueue.push(RecordPtr{});
        }

        {
            std::unique_lock l(m_mutexRQueue);
            m_pendingRecords += 1;
            m_pendingRecords -= discarded;
        }

        m_wQueueNotEmpty.notify_one();

        // wait until really flushed
        {
            auto stop = m_worker.get_stop_token();
            
            std::unique_lock l(m_mutexRQueue);
            return m_queuesEmpty.wait_for(l, stop, timeout, [this]() { return m_pendingRecords == 0; });
        }
    }
    
private:
    using AnyRecord = std::variant<RecordPtr, AtomicRecordPtr>;
    using RecordQueue = std::queue<AnyRecord>;

    void run(std::stop_token stop)
    {
        System::CurrentThread::setName("Logger");

        do
        {
            {
                std::unique_lock lw(m_mutexWQueue);
                
                if (!m_wQueueNotEmpty.wait_for(lw, stop, m_threshold, [this]() { return !m_wQueue.empty(); }))
                {
                    if (stop.stop_requested())
                        break;

                    continue;
                }
                
                // swap queues
                {
                    std::unique_lock lr(m_mutexRQueue);
                    m_rQueue.swap(m_wQueue);
                }

                // clean the oldest record's timestamp
                m_last = std::chrono::time_point<std::chrono::steady_clock>::min();
            }

            // wQueue is now unlocked - writers can go on
            sendToSinks(stop);

        } while (!stop.stop_requested());
    }

    void sendToSinks(std::stop_token stop)
    {
        std::unique_lock lr(m_mutexRQueue);

        auto count = m_rQueue.size();

        while (!m_rQueue.empty())
        {
            auto any = std::move(m_rQueue.front());
            m_rQueue.pop();

            auto r = std::get_if<RecordPtr>(&any);

            if (r)
            {
                auto record = *r;
                if (!record)
                    m_tee->flush(std::chrono::milliseconds(0)); // empty record means forced flush
                else
                    m_tee->write(record);
            }
            else
            {
                auto a = std::get_if<AtomicRecordPtr>(&any);
                if (a)
                    m_tee->write(*a);
            }

            if (stop.stop_requested())
                break;
        }

        m_pendingRecords -= count;
        if (m_pendingRecords == 0)
        {
            lr.unlock();
            m_queuesEmpty.notify_all();
        }
    }
    
    const std::chrono::milliseconds m_threshold;
    const std::int32_t MaxQueueSize;
    
    std::mutex m_mutexWQueue;
    std::condition_variable_any m_wQueueNotEmpty;
    RecordQueue m_wQueue;

    std::mutex m_mutexRQueue;
    std::int32_t m_pendingRecords = 0; 
    std::condition_variable_any m_queuesEmpty;
    RecordQueue m_rQueue;
    
    std::chrono::time_point<std::chrono::steady_clock> m_last = std::chrono::time_point<std::chrono::steady_clock>::min();
    std::jthread m_worker;
};


} // namespace {}


ER_RTL_EXPORT LoggerPtr makeLogger(std::string_view component, std::chrono::milliseconds threshold, std::int32_t maxQueueSize)
{
    return LoggerPtr(new AsyncLogger(component, threshold, maxQueueSize));
}

} // namespace Er::Log {}