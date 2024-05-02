#pragma once

#include <erebus/system/time.hxx>

#include <condition_variable>
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


struct Location
{
    const char* component = nullptr;
    const void* instance = nullptr;

    constexpr explicit Location(const char* component = nullptr, const void* instance = nullptr) noexcept
        : component(component)
        , instance(instance)
    {
    }
};


struct Record
{
    Level level = Level::Info;
    System::Time time;
    uintptr_t pid  = 0;
    uintptr_t tid = 0;
    Location location;
    std::string message;

    Record() noexcept = default;

    template <typename MessageT>
    explicit Record(Level level, const System::Time& time, uintptr_t pid, uintptr_t tid, const Location& location, MessageT&& message)
        : level(level)
        , time(time)
        , pid(pid)
        , tid(tid)
        , location(location)
        , message(std::forward<MessageT>(message))
    {
    }
};


using Delegate = std::function<void(std::shared_ptr<Record>)>;


struct ILog
{
    virtual Level level() const noexcept = 0;
    virtual bool writev(Level l, const Location& location, const char* format, va_list args) noexcept = 0;
    virtual bool write(Level l, const Location& location, const char* format, ...) noexcept = 0;
    virtual bool write(Level l, const Location& location, std::string_view s) noexcept = 0;
    virtual bool write(std::shared_ptr<Record> r) noexcept = 0;
    virtual void flush() noexcept = 0;

protected:
    virtual ~ILog() {}
};


struct ILogControl
{
    virtual void setLevel(Level l) noexcept = 0;
    virtual void addDelegate(std::string_view id, Delegate d) noexcept = 0;
    virtual void removeDelegate(std::string_view id) noexcept = 0;
    virtual void mute() noexcept = 0;
    virtual void unmute() noexcept = 0;

protected:
    virtual ~ILogControl() {}
};


class EREBUS_EXPORT LogBase
    : public ILog
    , public ILogControl
    , public Er::NonCopyable
{
public:
    struct SyncLogT
    {
    };

    struct AsyncLogT
    {
    };

    static constexpr SyncLogT SyncLog{};
    static constexpr AsyncLogT AsyncLog{};

    ~LogBase();
    explicit LogBase(AsyncLogT, Level level, size_t maxQueue = std::numeric_limits<size_t>::max()) noexcept;
    explicit LogBase(SyncLogT, Level level) noexcept;

    Level level() const noexcept override;
    bool writev(Level l, const Location& location, const char* format, va_list args) noexcept override;
    bool write(Level l, const Location& location, const char* format, ...) noexcept override;
    bool write(Level l, const Location& location, std::string_view s) noexcept override;
    bool write(std::shared_ptr<Record> r) noexcept override;
    void flush() noexcept override;

    void setLevel(Level l) noexcept override;
    void addDelegate(std::string_view id, Delegate d) noexcept override;
    void removeDelegate(std::string_view id) noexcept override;
    void mute() noexcept override;
    void unmute() noexcept override;
        
private:
    void _flush() noexcept;
    void run(std::stop_token stop) noexcept;

    bool m_sync;
    Level m_level;
    size_t m_maxQueue;
    std::mutex m_mutex;
    std::unordered_map<std::string, Delegate> m_delegates;
    std::queue<std::shared_ptr<Record>> m_queue;
    std::condition_variable_any m_event;
    std::jthread m_worker;
    bool m_mute = true;
};


class LogWrapperBase
    : public Er::NonCopyable
{
public:
    ~LogWrapperBase()
    {
        flush();
    }

    explicit LogWrapperBase(ILog* log, Level level, const Location& location) noexcept
        : m_log(log)
        , m_level(level)
        , m_location(location)
    {
    }

    void flush() noexcept
    {
        if (m_level < m_log->level())
            return;

        try
        {
            m_log->write(m_level, m_location, std::string_view(m_stream.str()));
            m_stream = std::ostringstream();
        }
        catch (...)
        {
        }
    }

    template <typename T>
    LogWrapperBase& operator<<(T&& v) noexcept
    {
        if (m_level < m_log->level())
            return *this;

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
    Location m_location;
    std::ostringstream m_stream;
};


class Debug final
    : public LogWrapperBase
{
public:
    explicit Debug(ILog* log, const Location& location = Location()) noexcept
        : LogWrapperBase(log, Level::Debug, location)
    {}
};

class Info final
    : public LogWrapperBase
{
public:
    explicit Info(ILog* log, const Location& location = Location()) noexcept
        : LogWrapperBase(log, Level::Info, location)
    {}
};

class Warning final
    : public LogWrapperBase
{
public:
    explicit Warning(ILog* log, const Location& location = Location()) noexcept
        : LogWrapperBase(log, Level::Warning, location)
    {}
};

class Error final
    : public LogWrapperBase
{
public:
    explicit Error(ILog* log, const Location& location = Location()) noexcept
        : LogWrapperBase(log, Level::Error, location)
    {}
};

class Fatal final
    : public LogWrapperBase
{
public:
    explicit Fatal(ILog* log, const Location& location = Location()) noexcept
        : LogWrapperBase(log, Level::Fatal, location)
    {}
};

} // namespace Log {}

} // namespace Er {}



#define ErLogDebug(log, ...) \
    (log->level() <= ::Er::Log::Level::Debug) && log->write(::Er::Log::Level::Debug, __VA_ARGS__)

#define ErLogInfo(log, ...) \
    (log->level() <= ::Er::Log::Level::Info) && log->write(::Er::Log::Level::Info, __VA_ARGS__)

#define ErLogWarning(log, ...) \
    (log->level() <= ::Er::Log::Level::Warning) && log->write(::Er::Log::Level::Warning, __VA_ARGS__)

#define ErLogError(log, ...) \
    (log->level() <= ::Er::Log::Level::Error) && log->write(::Er::Log::Level::Error, __VA_ARGS__)

#define ErLogFatal(log, ...) \
    (log->level() <= ::Er::Log::Level::Fatal) && log->write(::Er::Log::Level::Fatal, __VA_ARGS__)


#define ErLogNowhere() ::Er::Log::Location()
#define ErLogComponent(component) ::Er::Log::Location(component)
#define ErLogInstance(component) ::Er::Log::Location(component, this)

