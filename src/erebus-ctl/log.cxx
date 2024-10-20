#include "log.hxx"

#include <erebus/util/utf16.hxx>

#include <iostream>
#include <syncstream>

namespace ErCtl
{

Log::~Log()
{
    Er::Log::LogBase::flush();
    Er::Log::LogBase::removeDelegate("console");
}

Log::Log(Er::Log::Level level)
    : Er::Log::LogBase(Er::Log::LogBase::AsyncLog, level, 65536)
{
    Er::Log::LogBase::addDelegate("console", [this](std::shared_ptr<Er::Log::Record> r) { delegate(r); });
    Er::Log::LogBase::unmute();
}

void Log::delegate(std::shared_ptr<Er::Log::Record> r)
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
        "[%02d:%02d:%02d.%03d @%zu %s] ",
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

    if (r->level < Er::Log::Level::Error)
        std::osyncstream(std::cout) << message;
    else
        std::osyncstream(std::cerr) << message;
}

} // namespace ErCtl {}