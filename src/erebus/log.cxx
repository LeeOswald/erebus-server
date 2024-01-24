#include <erebus/log.hxx>
#include <erebus/system/process.hxx>


namespace Er
{
    
namespace Log
{

LogBase::~LogBase()
{
    flush();

    m_stop = true;
    m_event.set();

    if (m_worker.joinable())
        m_worker.join();
}

LogBase::LogBase(Level level, size_t maxQueue) noexcept
    : m_level(level)
    , m_maxQueue(maxQueue)
    , m_event(true)
    , m_worker([this]() { run(); })
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

void LogBase::run() noexcept
{
    try
    {
        while (!m_stop)
        {
            m_event.wait();

            flush();
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

    m_event.set();

    return true;
}

bool LogBase::write(Level l, std::string_view s) noexcept
{
    if (l < m_level)
        return false;

    auto pid = System::CurrentProcess::id();
#if ER_POSIX
    auto tid = static_cast<uintptr_t>(::gettid());
#elif ER_WINDOWS
    auto tid = static_cast<uintptr_t>(::GetCurrentThreadId());
#endif

    try
    {
        auto record = std::make_shared<Record>(l, System::Time::local(), pid, tid, s);
        return write(record);
    }
    catch (...)
    {
    }

    return false;
}

bool LogBase::writev(Level l, const char* format, va_list args) noexcept
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

        return write(l, std::string_view(formatted));
    }
    catch (...)
    {
    }

    return false;
}

bool LogBase::write(Level l, const char* format, ...) noexcept
{
    if (l < m_level)
        return false;

    va_list args;
    va_start(args, format);

    auto b = writev(l, format, args);

    va_end(args);

    return b;
}


} // namespace Log {}   
    
} // namespace Er {}