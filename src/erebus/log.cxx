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
        std::lock_guard g(m_mutex);

        m_delegates.insert({ std::string(id), d });
    }
    catch (...)
    {
    }
}

void LogBase::removeDelegate(std::string_view id) noexcept
{
    try
    {
        std::lock_guard g(m_mutex);

        auto it = m_delegates.find(std::string(id));
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
            std::unique_lock l(m_mutex);

            m_event.wait(l, stop, [this]() { return !m_queue.empty(); });

            _flush();
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

    while (!m_queue.empty())
    {
        for (auto it = m_delegates.begin(); it != m_delegates.end(); ++it)
        {
            try
            {
                auto record = m_queue.front();
                it->second(record);
            }
            catch (...)
            {
            }
        }

        m_queue.pop();
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
    std::lock_guard g(m_mutex);

    _flush();
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
        for (auto it = m_delegates.begin(); it != m_delegates.end(); ++it)
        {
            try
            {
                it->second(r);
            }
            catch (...)
            {
            }
        }
    }
    else
    {
        std::lock_guard g(m_mutex);

        try
        {
            m_queue.push(r);
        }
        catch (...)
        {
            return false;
        }

        // drop the older events to avoid the queue overflow
        while (m_queue.size() > m_maxQueue)
            m_queue.pop();

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