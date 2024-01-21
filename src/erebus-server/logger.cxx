#include "logger.hxx"

#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>
#include <erebus/time.hxx>

#if ER_POSIX
    #include <fcntl.h>
    #include <sys/file.h>

    #include <erebus/util/posixerror.hxx>
#endif

#if ER_WINDOWS
    #include <erebus/util/win32error.hxx>
#endif


#include <iostream>
#include <syncstream>


namespace Er
{

namespace Private
{

Logger::~Logger()
{
    // we have to flush here before we remove our delegate
    flush();
    removeDelegate("this");
}

Logger::Logger(Er::Log::Level level, const char* fileName)
    : Er::Log::LogBase(level, 65536)
#if ER_POSIX
    , m_file(::open(fileName, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH))
#elif ER_WINDOWS
    , m_file(::CreateFileA(fileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0))
#endif
{
#if ER_POSIX
    if (m_file == -1)
    {
        std::osyncstream(std::cerr) << "Failed to create the logfile: " << errno << "\n";
    }
    else if (::flock(m_file, LOCK_EX | LOCK_NB) == -1)
    {
        if (errno == EWOULDBLOCK)
        {
            m_exclusive = false;
            std::osyncstream(std::cerr) << "Server is already running\n";
        }
        else
        {
            std::osyncstream(std::cerr) << "Failed to lock the logfile: " << errno << "\n";
        }
    }
#elif ER_WINDOWS
    if (m_file == INVALID_HANDLE_VALUE)
    {
        auto e = ::GetLastError();
        if (e == ERROR_SHARING_VIOLATION)
        {
            m_exclusive = false;
            std::osyncstream(std::cerr) << "Server is already running\n";
        }
        else
        {
            std::osyncstream(std::cerr) << "Failed to create the logfile: " << e << "\n";
        }
    }
#endif

    addDelegate("this", [this](std::shared_ptr<Er::Log::Record> r) { delegate(r); });
}

void Logger::delegate(std::shared_ptr<Er::Log::Record> r) 
{
    const char* strLevel = "?";
    switch (r->level)
    {
    case Er::Log::Level::Debug: strLevel = "D"; break;
    case Er::Log::Level::Info: strLevel = "I"; break;
    case Er::Log::Level::Warning: strLevel = "W"; break;
    case Er::Log::Level::Error: strLevel = "E"; break;
    case Er::Log::Level::Fatal: strLevel = "!"; break;
    }
        
    char prefix[256];
    ::snprintf(prefix, _countof(prefix), "[%02d:%02d:%02d.%03d %s] ", r->time.hour, r->time.minute, r->time.second, r->time.milli, strLevel);

    std::string message = std::string(prefix);
    message.append(r->message);
    message.append("\n");

    bool fileValid = false;
#if ER_POSIX
    if (m_file < 0)
#elif ER_WINDOWS
    if (m_file == INVALID_HANDLE_VALUE)
#endif
    {
        // if we failed to create a logfile we have nothing else than std::cerr
        std::osyncstream(std::cerr) << message;
    }
    else
    {
        fileValid = true;
#if ER_DEBUG
        // for debug purposes use std::cout
        if (r->level < Er::Log::Level::Warning)
            std::osyncstream(std::cout) << message;
        else
            std::osyncstream(std::cerr) << message;
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

bool Logger::exclusive() const noexcept
{
    return m_exclusive;
}

} // namespace Private {}

} // namespace Er {}
