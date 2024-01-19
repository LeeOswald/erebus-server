#include <erebus/log.hxx>

#include <iomanip>


namespace Er
{
    
namespace Log
{
    
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
    m_hex = true;
    return *this;
}

LogWrapperBase& LogWrapperBase::operator<<(Width w) noexcept
{
    m_width = w.width;
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

void LogWrapperBase::applyOptions()
{
    if (m_hex)
    {
        m_stream << std::hex;
        m_hex = false;
    }

    if (m_width)
    {
        m_stream << std::setw(*m_width) << std::setfill('0');
        m_width = std::nullopt;
    }
}

LogWrapperBase& LogWrapperBase::operator<<(int16_t i) noexcept
{
    try
    {
        applyOptions();
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
        applyOptions();
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
        applyOptions();
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
        applyOptions();
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
        applyOptions();
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
        applyOptions();
        m_stream << u;
    }
    catch (...)
    {
    }
    return *this;
}


} // namespace Log {}   
    
} // namespace Er {}