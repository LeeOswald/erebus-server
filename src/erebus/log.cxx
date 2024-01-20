#include <erebus/log.hxx>

#include <iomanip>


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

    m_event.set();

    // drop the older events to avoid the queue overflow
    while (m_queue.size() > m_maxQueue)
        m_queue.pop();

    return true;
}

bool LogBase::write(Level l, std::string_view s) noexcept
{
    if (l < m_level)
        return false;

#if ER_POSIX
    auto pid = static_cast<uintptr_t>(::getpid());
    auto tid = static_cast<uintptr_t>(::gettid());
#elif ER_WINDOWS
    auto pid = static_cast<uintptr_t>(::GetCurrentProcessId());
    auto tid = static_cast<uintptr_t>(::GetCurrentThreadId());
#endif

    try
    {
        auto record = std::make_shared<Record>(l, Time::local(), pid, tid, s);
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
    
LogWrapperBase::~LogWrapperBase()
{
    flush();
}

LogWrapperBase::LogWrapperBase(ILog* log, Level level) noexcept
    : m_log(log)
    , m_level(level)
{
}

void LogWrapperBase::flush() noexcept
{
    try
    {
        m_log->write(m_level, std::string_view(m_stream.str()));
        m_stream = std::ostringstream();
    }
    catch (...)
    {
    }
}

void LogWrapperBase::write(std::string_view s) noexcept
{
    try
    {
        m_stream << s;
    }
    catch (...)
    {
    }
}

LogWrapperBase& LogWrapperBase::operator<<(nullptr_t) noexcept
{
    write("(nullptr)");
    return *this;
}

LogWrapperBase& LogWrapperBase::operator<<(char c) noexcept
{
    write(std::string_view(&c, 1));
    return *this;
}

LogWrapperBase& LogWrapperBase::operator<<(const char* s) noexcept
{
    if (s)
        write(s);
    else
        write(nullptr);

    return *this;
}

LogWrapperBase& LogWrapperBase::operator<<(std::string_view s) noexcept
{
    write(s);
    return *this;
}

LogWrapperBase& LogWrapperBase::operator<<(const std::string& s) noexcept
{
    write(s);
    return *this;
}

LogWrapperBase& LogWrapperBase::operator<<(bool b) noexcept
{
    write(b ? "true" : "false");
    return *this;
}

LogWrapperBase& LogWrapperBase::operator<<(Hex) noexcept
{
    try
    {
        m_stream << std::hex;
    }
    catch (...)
    {
    }
    return *this;
}

LogWrapperBase& LogWrapperBase::operator<<(Dec) noexcept
{
    try
    {
        m_stream << std::dec;
    }
    catch (...)
    {
    }
    return *this;
}

LogWrapperBase& LogWrapperBase::operator<<(Width w) noexcept
{
    try
    {
        m_stream << std::setw(w.width) << std::setfill('0');
    }
    catch (...)
    {
    }
    return *this;
}

LogWrapperBase& LogWrapperBase::operator<<(const void* v) noexcept
{
    try
    {
        m_stream << v;
    }
    catch (...)
    {
    }
    return *this;
}

LogWrapperBase& LogWrapperBase::operator<<(int16_t i) noexcept
{
    try
    {
        m_stream << i;
    }
    catch (...)
    {
    }
    return *this;
}

LogWrapperBase& LogWrapperBase::operator<<(uint16_t u) noexcept
{
    try
    {
        m_stream << u;
    }
    catch (...)
    {
    }
    return *this;
}

LogWrapperBase& LogWrapperBase::operator<<(int32_t i) noexcept
{
    try
    {
        m_stream << i;
    }
    catch (...)
    {
    }
    return *this;
}

LogWrapperBase& LogWrapperBase::operator<<(uint32_t u) noexcept
{
    try
    {
        m_stream << u;
    }
    catch (...)
    {
    }
    return *this;
}

LogWrapperBase& LogWrapperBase::operator<<(int64_t i) noexcept
{
    try
    {
        m_stream << i;
    }
    catch (...)
    {
    }
    return *this;
}

LogWrapperBase& LogWrapperBase::operator<<(uint64_t u) noexcept
{
    try
    {
        m_stream << u;
    }
    catch (...)
    {
    }
    return *this;
}

LogWrapperBase& LogWrapperBase::operator<<(Float f) noexcept
{
    try
    {
        m_stream << std::fixed << std::setprecision(f.precision);
    }
    catch (...)
    {
    }
    return *this;
}

LogWrapperBase& LogWrapperBase::operator<<(float f) noexcept
{
    try
    {
        m_stream << f;
    }
    catch (...)
    {
    }
    return *this;
}

LogWrapperBase& LogWrapperBase::operator<<(double d) noexcept
{
    try
    {
        m_stream << d;
    }
    catch (...)
    {
    }
    return *this;
}


} // namespace Log {}   
    
} // namespace Er {}