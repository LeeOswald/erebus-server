#include <erebus/log.hxx>
#include <erebus/system/process.hxx>
#include <erebus/system/thread.hxx>

namespace Er
{
    
namespace Log
{

LogBase::~LogBase()
{
}

LogBase::LogBase(AsyncLogT mode, Level level, size_t maxQueue) noexcept
    : m_sync(false)
    , m_level(level)
    , m_maxQueue(maxQueue)
    , m_worker([this](std::stop_token stop) { run(stop); })
{
}

LogBase::LogBase(SyncLogT mode, Level level) noexcept
    : m_sync(true)
    , m_level(level)
    , m_maxQueue(1024) // ignored
    , m_worker()
{
}

void LogBase::addDelegate(std::string_view id, Delegate d) noexcept
{
    try
    {
        std::lock_guard g(m_delegatesMutex);

        m_delegates.emplace_back(id, d);
    }
    catch (...)
    {
    }
}

void LogBase::removeDelegate(std::string_view id) noexcept
{
    try
    {
        std::lock_guard g(m_delegatesMutex);

        auto it = std::find_if(m_delegates.begin(), m_delegates.end(), [id](const DelegateInfo& d) { return d.id == id; });
        if (it != m_delegates.end())
        {
            m_delegates.erase(it);
        }
    }
    catch (...)
    {
    }
}

void LogBase::run(std::stop_token stop) noexcept
{
    try
    {
        System::CurrentThread::setName("Logger");
        
        while (!stop.stop_requested())
        {
            // wait on write queue
            {
                std::unique_lock lw(m_wQueueMutex);

                m_event.wait(lw, stop, [this]() { return !m_wQueue.empty(); });

                // swap read and write queues
                {
                    std::unique_lock lr(m_rQueueMutex);
                    m_rQueue.swap(m_wQueue);
                }
            }

            // write queue is now unlocked
            // drain the read queue
            {
                std::unique_lock lr(m_rQueueMutex);
                _flush();
            }
        }
    }
    catch (...)
    {
    }
}

void LogBase::_flush() noexcept
{
    if (m_mute)
        return;

    std::lock_guard l(m_delegatesMutex);

    while (!m_rQueue.empty())
    {
        for (auto it = m_delegates.begin(); it != m_delegates.end(); ++it)
        {
            auto record = m_rQueue.front();

            try
            {
                it->d(record);
            }
            catch (...)
            {
            }
        }

        m_rQueue.pop();
    }
}

void LogBase::mute() noexcept
{
    m_mute = true;
}

void LogBase::unmute() noexcept
{
    m_mute = false;
}

void LogBase::flush() noexcept
{
    // drain the read queue
    {
        std::unique_lock l(m_rQueueMutex);

        _flush();
    }

    {
        std::unique_lock lw(m_wQueueMutex);

        // swap read and write queues
        {
            std::unique_lock lr(m_rQueueMutex);
            m_rQueue.swap(m_wQueue);
        }
    }

    // flush everything that was in the former write queue
    {
        std::unique_lock l(m_rQueueMutex);

        _flush();
    }

}

Level LogBase::level() const noexcept
{
    return m_level;
}

bool LogBase::write(std::shared_ptr<Record> r) noexcept
{
    if (r->level < m_level)
        return false;

    if (!r)
        return true;
    
    if (m_sync)
    {
        std::lock_guard l(m_delegatesMutex);

        for (auto it = m_delegates.begin(); it != m_delegates.end(); ++it)
        {
            try
            {
                it->d(r);
            }
            catch (...)
            {
            }
        }
    }
    else
    {
        std::unique_lock l(m_wQueueMutex);

        try
        {
            m_wQueue.push(r);
        }
        catch (...)
        {
            return false;
        }

        // drop the older events to avoid the queue overflow
        while (m_wQueue.size() > m_maxQueue)
            m_wQueue.pop();

    }

    if (!m_sync)
    {
        // do this outside the lock
        m_event.notify_one();
    }

    return true;
}

bool LogBase::write(Level l, const Location& location, std::string_view s) noexcept
{
    if (l < m_level)
        return false;

    auto pid = System::CurrentProcess::id();
    auto tid = System::CurrentThread::id();

    try
    {
        auto record = std::make_shared<Record>(l, System::Time::local(), pid, tid, location, s);
        return write(record);
    }
    catch (...)
    {
    }

    return false;
}

bool LogBase::writev(Level l, const Location& location, const char* format, va_list args) noexcept
{
    if (l < m_level)
        return false;

    try
    {
        va_list args1;
        va_copy(args1, args);
        auto required = ::vsnprintf(nullptr, 0, format, args1);

        std::string formatted;
        formatted.resize(required);
        ::vsnprintf(formatted.data(), required + 1, format, args);

        va_end(args1);

        return write(l, location, std::string_view(formatted));
    }
    catch (...)
    {
    }

    return false;
}

bool LogBase::write(Level l, const Location& location, const char* format, ...) noexcept
{
    if (l < m_level)
        return false;

    va_list args;
    va_start(args, format);

    auto b = writev(l, location, format, args);

    va_end(args);

    return b;
}

void LogBase::setLevel(Level l) noexcept
{
    m_level = l;
}

} // namespace Log {}   
    
} // namespace Er {}