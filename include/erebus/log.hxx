#pragma once

#include <erebus/time.hxx>
#include <erebus/util/condition.hxx>

#include <functional>
#include <iomanip>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
#include <unordered_map>

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
    Off // should go last
};


struct Record
{
    Level level = Level::Info;
    Time time;
    uintptr_t pid  = 0;
    uintptr_t tid = 0;
    std::string message;

    constexpr Record() noexcept = default;

    template <typename MessageT>
    explicit constexpr Record(Level level, const Time& time, uintptr_t pid, uintptr_t tid, MessageT&& message)
        : level(level)
        , time(time)
        , pid(pid)
        , tid(tid)
        , message(std::forward<MessageT>(message))
    {
    }
};


using Delegate = std::function<void(std::shared_ptr<Record>)>;


struct ILog
{
    virtual Level level() const noexcept = 0;
    virtual bool writev(Level l, const char* format, va_list args) noexcept = 0;
    virtual bool write(Level l, const char* format, ...) noexcept = 0;
    virtual bool write(Level l, std::string_view s) noexcept = 0;
    virtual bool write(std::shared_ptr<Record> r) noexcept = 0;

protected:
    virtual ~ILog() {}
};


class EREBUS_EXPORT LogBase
    : public ILog
    , public boost::noncopyable
{
public:
    ~LogBase();
    explicit LogBase(Level level, size_t maxQueue = std::numeric_limits<size_t>::max()) noexcept;

    void addDelegate(std::string_view id, Delegate d) noexcept;
    void removeDelegate(std::string_view id) noexcept;
    void mute() noexcept;
    void unmute() noexcept;

    Level level() const noexcept override;
    bool writev(Level l, const char* format, va_list args) noexcept override;
    bool write(Level l, const char* format, ...) noexcept override;
    bool write(Level l, std::string_view s) noexcept override;
    bool write(std::shared_ptr<Record> r) noexcept override;
    void flush() noexcept;

private:
    void _flush() noexcept;
    void run() noexcept;

    Level m_level;
    size_t m_maxQueue;
    std::mutex m_mutex;
    std::unordered_map<std::string, Delegate> m_delegates;
    std::queue<std::shared_ptr<Record>> m_queue;
    Util::Condition m_event;
    bool m_stop = false;
    std::thread m_worker;
    bool m_mute = true;
};


class LogWrapperBase
    : public boost::noncopyable
{
public:
    ~LogWrapperBase()
    {
        flush();
    }

    explicit LogWrapperBase(ILog* log, Level level) noexcept
        : m_log(log)
        , m_level(level)
    {
    }

    void flush() noexcept
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

    template <typename T>
    LogWrapperBase& operator<<(T&& v) noexcept
    {
        try
        {
            m_stream << std::forward<T>(v);
        }
        catch (...)
        {
        }
        return *this;
    }

private:
    ILog* m_log;
    Level m_level;
    std::ostringstream m_stream;
};


class Debug final
    : public LogWrapperBase
{
public:
    explicit Debug(ILog* log) noexcept
        : LogWrapperBase(log, Level::Debug)
    {}
};

class Info final
    : public LogWrapperBase
{
public:
    explicit Info(ILog* log) noexcept
        : LogWrapperBase(log, Level::Info)
    {}
};

class Warning final
    : public LogWrapperBase
{
public:
    explicit Warning(ILog* log) noexcept
        : LogWrapperBase(log, Level::Warning)
    {}
};

class Error final
    : public LogWrapperBase
{
public:
    explicit Error(ILog* log) noexcept
        : LogWrapperBase(log, Level::Error)
    {}
};

class Fatal final
    : public LogWrapperBase
{
public:
    explicit Fatal(ILog* log) noexcept
        : LogWrapperBase(log, Level::Fatal)
    {}
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


