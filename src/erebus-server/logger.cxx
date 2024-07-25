#include "logger.hxx"

#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>
#include <erebus/syncstream.hxx>
#include <erebus/system/time.hxx>
#include <erebus/util/format.hxx>
#include <erebus/util/utf16.hxx>

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

LogRotator::LogRotator(Er::Log::Level level, const char* fileName, int keep)
    : Er::Log::LogBase(Er::Log::LogBase::AsyncLog, level, 65536)
{
    int i = keep;
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
    Er::Log::LogBase::flush();
    Er::Log::LogBase::removeDelegate("this");
}

Logger::Logger(Er::Log::Level level, const char* fileName, int keep)
    : LogRotator(level, fileName, keep)
#if ER_POSIX
    , m_file(::open(fileName, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH))
#elif ER_WINDOWS
    , m_file(::CreateFileW(Er::Util::utf8ToUtf16(fileName).c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0))
#endif
{
#if ER_POSIX
    if (m_file == -1)
        throwPosixError("Failed to create the logfile", errno, Er::ExceptionProps::FileName(fileName));
    
#elif ER_WINDOWS
    if (m_file == INVALID_HANDLE_VALUE)
        throwWin32Error("Failed to create the logfile", ::GetLastError(), Er::ExceptionProps::FileName(fileName));
        
#endif

    Er::Log::LogBase::addDelegate("this", [this](std::shared_ptr<Er::Log::Record> r) { delegate(r); });
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
        "[%02d.%02d.%04d %02d:%02d:%02d.%03d @%zu %s] ",
        r->time.day,
        r->time.month,
        r->time.year,
        r->time.hour, 
        r->time.minute, 
        r->time.second,
        r->time.milli,
        r->tid,
        strLevel
    );


    std::string message = std::string(prefix);
    message.append(r->message);
    message.append("\n");

#if ER_WINDOWS && ER_DEBUG
    ::OutputDebugStringW(Er::Util::utf8ToUtf16(message).c_str());
#endif


#if ER_DEBUG
    // for debug purposes use std::cout
    if (r->level < Er::Log::Level::Warning)
        Er::osyncstream(std::cout) << message;
    else
        Er::osyncstream(std::cerr) << message;
#endif

#if ER_POSIX
    ::write(m_file, message.data(), message.length());
    ::fdatasync(m_file);
#elif ER_WINDOWS
    DWORD written = 0;
    ::WriteFile(m_file, message.data(), message.length(), &written, nullptr);
    ::FlushFileBuffers(m_file);
#endif
}


} // namespace Private {}

} // namespace Er {}
