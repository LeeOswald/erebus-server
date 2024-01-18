#pragma once

#include <erebus/erebus.hxx>


namespace Er
{

namespace Log
{

enum class Level
{
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
    Off // shoud go last
};


struct ILog
{
    virtual Level level() const noexcept = 0;
    virtual bool writev(Level l, const char* format, va_list args) noexcept = 0;
    virtual bool write(Level l, const char* format, ...) noexcept = 0;
    virtual bool write(Level l, std::string_view s) noexcept = 0;

protected:
    virtual ~ILog() {}
};


} // namespace Log {}

} // namespace Er {}



#define LogDebug(log, ...) \
    (log->level() <= Er::Log::Level::Debug) && log->write(Er::Log::Level::Debug, __VA_ARGS__)

#define LogInfo(log, ...) \
    (log->level() <= Er::Log::Level::Info) && log->write(Er::Log::Level::Info, __VA_ARGS__)

#define LogWarning(log, ...) \
    (log->level() <= Er::Log::Level::Warning) && log->write(Er::Log::Level::Warning, __VA_ARGS__)

#define LogError(log, ...) \
    (log->level() <= Er::Log::Level::Error) && log->write(Er::Log::Level::Error, __VA_ARGS__)

#define LogFatal(log, ...) \
    (log->level() <= Er::Log::Level::Fatal) && log->write(Er::Log::Level::Fatal, __VA_ARGS__)


