#include "logger.hxx"

#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>
#include <erebus/syncstream.hxx>
#include <erebus/system/time.hxx>

#if ER_POSIX
    #include <fcntl.h>
    #include <sys/file.h>

    #include <erebus/util/posixerror.hxx>
#endif

#if ER_WINDOWS
    #include <erebus/util/utf16.hxx>
    #include <erebus/util/win32error.hxx>
#endif


#include <iostream>


namespace Er
{

namespace Private
{

LogRotator::LogRotator(Er::Log::Level level, const char* fileName)
    : Er::Log::LogBase(level, 65536)
{
    int i = kKeep;
    while (i >= 0)
    {
        std::string nameNew = std::string(fileName) + std::string(".") + std::to_string(i);
        std::remove(nameNew.c_str());
        std::string nameOld = std::string(fileName);
        if (i > 0)
        {
            nameOld.append(".");
            nameOld.append(std::to_string(i - 1));
        }
        else
        {
            nameOld = fileName;
        }

        auto status = std::rename(nameOld.c_str(), nameNew.c_str());
        if (status != 0)
        {
            // nothing we can do here
        }

        --i;
    }
}


Logger::~Logger()
{
    // we have to flush here before we remove our delegate
    flush();
    removeDelegate("this");
}

Logger::Logger(Er::Log::Level level, const char* fileName)
    : LogRotator(level, fileName)
#if ER_POSIX
    , m_file(::open(fileName, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH))
#elif ER_WINDOWS
    , m_file(::CreateFileW(Er::Util::utf8ToUtf16(fileName).c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0))
#endif
{
#if ER_POSIX
    if (m_file == -1)
    {
        Er::osyncstream(std::cerr) << "Failed to create the logfile: " << errno << "\n";
    }
    else if (::flock(m_file, LOCK_EX | LOCK_NB) == -1)
    {
        if (errno == EWOULDBLOCK)
        {
            m_exclusive = false;
            Er::osyncstream(std::cerr) << "Server is already running\n";
        }
        else
        {
            Er::osyncstream(std::cerr) << "Failed to lock the logfile: " << errno << "\n";
        }
    }
#elif ER_WINDOWS
    if (m_file == INVALID_HANDLE_VALUE)
    {
        auto e = ::GetLastError();
        if (e == ERROR_SHARING_VIOLATION)
        {
            m_exclusive = false;
            Er::osyncstream(std::cerr) << "Server is already running\n";
        }
        else
        {
            Er::osyncstream(std::cerr) << "Failed to create the logfile: " << e << "\n";
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
    ::snprintf(prefix, 
        _countof(prefix), 
        "[%02d.%02d.%04d %02d:%02d:%02d.%03d @%zu:%zu %s] ",
        r->time.day,
        r->time.month,
        r->time.year,
        r->time.hour, 
        r->time.minute, 
        r->time.second,
        r->time.milli,
        r->pid,
        r->tid,
        strLevel
    );

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
        Er::osyncstream(std::cerr) << message;
    }
    else
    {
        fileValid = true;
#if ER_DEBUG
        // for debug purposes use std::cout
        if (r->level < Er::Log::Level::Warning)
            Er::osyncstream(std::cout) << message;
        else
            Er::osyncstream(std::cerr) << message;
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
