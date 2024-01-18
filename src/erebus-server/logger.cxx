#include "logger.hxx"

#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>

#if ER_POSIX
    #include <fcntl.h>
    #include <sys/file.h>

    #include <erebus/util/posixerror.hxx>
#endif

#if ER_WINDOWS
    #include <erebus/util/win32error.hxx>
#endif

#include <ctime>
#include <time.h>

#include <iostream>

namespace Er
{

namespace Private
{

Logger::~Logger()
{
}

Logger::Logger(Er::Log::Level level, const char* fileName)
#if ER_POSIX
    : m_file(::open(fileName, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH))
#elif ER_WINDOWS
    : m_file(::CreateFileA(fileName, GENERIC_READ, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0))
#endif
    , m_level(level)
{
#if ER_POSIX
    if (m_file == -1)
    {
        std::cerr << "Failed to create the logfile: " << errno << "\n";
    }
    else if (::flock(m_file, LOCK_EX | LOCK_NB) == -1)
    {
        if (errno == EWOULDBLOCK)
        {
            m_exclusive = false;
            std::cerr << "Server is already running\n";
        }
        else
        {
            std::cerr << "Failed to lock the logfile: " << errno << "\n";
        }
    }
#elif ER_WINDOWS
    if (m_file == INVALID_HANDLE_VALUE)
    {
        auto e = ::GetLastError();
        if (e == ERROR_SHARING_VIOLATION)
        {
            m_exclusive = false;
            std::cerr << "Server is already running\n";
        }
        else
        {
            std::cerr << "Failed to create the logfile: " << e << "\n";
        }
    }
#endif
}

Er::Log::Level Logger::level() const noexcept
{
    return m_level;
}

bool Logger::write(Er::Log::Level level, std::string_view s) noexcept
{
    if (level < m_level)
        return true;

    try
    {
        const char* strLevel = "?";
        switch (level)
        {
        case Er::Log::Level::Debug: strLevel = "D"; break;
        case Er::Log::Level::Info: strLevel = "I"; break;
        case Er::Log::Level::Warning: strLevel = "W"; break;
        case Er::Log::Level::Error: strLevel = "E"; break;
        case Er::Log::Level::Fatal: strLevel = "!"; break;
        }

#if ER_POSIX
        struct tm localNow = {};
        struct timespec now = {};
        ::clock_gettime(CLOCK_REALTIME, &now);

        // round nanoseconds to milliseconds
        long msec = 0;
        if (now.tv_nsec >= 999500000)
        {
            now.tv_sec++;
            msec = 0;
        }
        else
        {
            msec = (now.tv_nsec + 500000) / 1000000;
        }

        ::localtime_r(&now.tv_sec, &localNow);

        char prefix[256];
        ::snprintf(prefix, _countof(prefix), "[%02d:%02d:%02d.%03d %s] ", localNow.tm_hour, localNow.tm_min, localNow.tm_sec, msec, strLevel);

#elif ER_WINDOWS
        SYSTEMTIME time = {};
        ::GetLocalTime(&time);

        char prefix[256];
        ::snprintf(prefix, _countof(prefix), "[%02d:%02d:%02d.%03d %s] ", time.wHour, time.wMinute, time.wSecond, time.wMilliseconds, strLevel);
#endif

        std::string message = std::string(prefix);
        message.append(s);
        message.append("\n");

        bool fileValid = false;
#if ER_POSIX
        if (m_file < 0)
#elif ER_WINDOWS
        if (m_file == INVALID_HANDLE_VALUE)
#endif
        {
            std::cout << message;
        }
        else
        {
            fileValid = true;
#if ER_DEBUG
            std::cout << message;
#endif
        }

        if (fileValid)
        {
#if ER_POSIX
            ::write(m_file, message.data(), message.length());
            ::fsync(m_file);
#elif ER_WINDOWS
            DWORD written = 0;
            ::WriteFile(m_file, message.data(), message.length(), &written, nullptr);
            ::FlushFileBuffers(m_file);
#endif
        }
    }
    catch (...)
    {
        // not much we can do here
    }

    return true;
}

bool Logger::exclusive() const noexcept
{
    return m_exclusive;
}

bool Logger::writev(Er::Log::Level level, const char* format, va_list args) noexcept
{
    if (level < m_level)
        return true;

    try
    {
        va_list args1;
        va_copy(args1, args);
        auto required = ::vsnprintf(nullptr, 0, format, args1);

        std::string formatted;
        formatted.resize(required);
        ::vsnprintf(formatted.data(), required + 1, format, args);

        va_end(args1);

        return write(level, std::move(formatted));
    }
    catch (...)
    {
        // not much we can do here
    }

    return true;
}

bool Logger::write(Er::Log::Level level, const char* format, ...) noexcept
{
    if (level < m_level)
        return true;

    va_list args;
    va_start(args, format);

    auto result = writev(level, format, args);

    va_end(args);

    return result;
}


} // namespace Private {}

} // namespace Er {}
