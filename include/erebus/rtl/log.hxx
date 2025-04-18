#pragma once

#include <erebus/rtl/format.hxx>
#include <erebus/rtl/system/packed_time.hxx>
#include <erebus/rtl/system/thread.hxx>

#include <boost/noncopyable.hpp>

#include <chrono>
#include <functional>
#include <vector>


namespace Er::Log
{

enum class Level
{
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
    Off
};


struct Record
    : public boost::noncopyable
{
private:
    struct PrivateOnly{};

public:
    using Ptr = std::shared_ptr<Record>;

    Record(PrivateOnly, Level level, System::PackedTime::ValueType time, uintptr_t tid, auto&& message)
        : m_level(level)
        , m_time(time)
        , m_tid(tid)
        , m_message(std::forward<decltype(message)>(message))
    {
    }

    Record(PrivateOnly, std::string_view component, Level level, System::PackedTime::ValueType time, uintptr_t tid, auto&& message)
        : m_component(component)
        , m_level(level)
        , m_time(time)
        , m_tid(tid)
        , m_message(std::forward<decltype(message)>(message))
    {
    }

    [[nodiscard]] constexpr auto level() const noexcept
    {
        return m_level;
    }

    [[nodiscard]] constexpr auto time() const noexcept
    {
        return m_time;
    }

    [[nodiscard]] constexpr auto tid() const noexcept
    {
        return m_tid;
    }

    [[nodiscard]] constexpr const auto& component() const noexcept
    {
        return m_component;
    }

    [[nodiscard]] constexpr const auto& message() const noexcept
    {
        return m_message;
    }

    [[nodiscard]] constexpr auto indent() const noexcept
    {
        return m_indent;
    }

    void setComponent(std::string_view component) noexcept
    {
        // use in the top level logger only
        // since the structure may be shared between many consumers
        m_component = component;
    }

    void setIndent(unsigned indent) noexcept
    {
        // use in the top level logger only
        // since the structure may be shared between many consumers
        m_indent = indent;
    }

    [[nodiscard]] static auto make(Level level, System::PackedTime::ValueType time, uintptr_t tid, auto&& message)
    {
        return std::make_shared<Record>(PrivateOnly{}, level, time, tid, std::forward<decltype(message)>(message));
    }

    [[nodiscard]] static auto make(std::string_view component, Level level, System::PackedTime::ValueType time, uintptr_t tid, auto&& message)
    {
        return std::make_shared<Record>(PrivateOnly{}, component, level, time, tid, std::forward<decltype(message)>(message));
    }

private:
    std::string_view m_component;
    const Level m_level = Level::Info;
    const System::PackedTime::ValueType m_time;
    const uintptr_t m_tid = 0;
    const std::string m_message;
    unsigned m_indent = 0;
};


using AtomicRecord = std::vector<Record::Ptr>;


struct IFormatter
{
    using Ptr = std::unique_ptr<IFormatter>;

    virtual ~IFormatter() = default;

    [[nodiscard]] virtual std::string format(const Record* r) const = 0;
};


struct NullFormatter
    : public IFormatter
{
private:
    struct PrivateOnly{};

public:
    NullFormatter(PrivateOnly) noexcept
    {
    }

    [[nodiscard]] std::string format(const Record* r) const override
    {
        return r->message();
    }

    [[nodiscard]] static auto make()
    {
        return std::make_shared<NullFormatter>(PrivateOnly{});
    }
};


using Filter = std::function<bool(const Record*)>;


struct ISink
{
    using Ptr = std::shared_ptr<ISink>;

    virtual ~ISink() = default;

    virtual void write(Record::Ptr r) = 0;
    virtual void write(AtomicRecord a) = 0;
    virtual void flush() = 0;
};


struct SinkBase
    : public ISink
    , public boost::noncopyable
{
    SinkBase(IFormatter::Ptr&& formatter, auto&& filter) noexcept
        : m_formatter(std::move(formatter))
        , m_filter(std::forward<decltype(filter)>(filter))
    {}

    bool filter(const Record* r) const 
    {
        if (m_filter && !m_filter(r))
            return false;

        return true;
    }

    std::string format(const Record* r) const
    {
        return m_formatter->format(r);
    }

private:
    IFormatter::Ptr m_formatter;
    Filter m_filter;
};


struct ITee
    : public ISink
{
    using Ptr = std::shared_ptr<ITee>;

    virtual ~ITee() = default;

    virtual void addSink(std::string_view name, ISink::Ptr sink) = 0;
    virtual void removeSink(std::string_view name) = 0;
    virtual ISink::Ptr findSink(std::string_view name) = 0;
};


ER_RTL_EXPORT ITee::Ptr makeTee(ThreadSafe mode);


struct ILogger
    : public ITee
{
    using Ptr = std::shared_ptr<ILogger>;

    virtual ~ILogger() = default;

    Level level() const noexcept
    {
        return m_level;
    }

    Level setLevel(Level level) noexcept
    {
        auto prev = m_level;
        m_level = level;
        return prev;
    }

    virtual void indent() noexcept = 0;
    virtual void unindent() noexcept = 0;
    virtual void beginBlock() noexcept = 0;
    virtual void endBlock() noexcept = 0;

protected:
    Level m_level = Level::Debug;
};

ER_RTL_EXPORT ILogger::Ptr makeLogger(std::string_view component = {}, std::chrono::milliseconds threshold = {});
ER_RTL_EXPORT ILogger::Ptr makeSyncLogger(std::string_view component = {});


struct AtomicBlock
    : public boost::noncopyable
{
    ~AtomicBlock()
    {
        m_log->endBlock();
    }

    AtomicBlock(ILogger* log)
        : m_log(log)
    {
        ErAssert(log);
        log->beginBlock();
    }

private:
    ILogger* m_log;
};


struct IndentScope
    : public boost::noncopyable
{
    ~IndentScope()
    {
        if (m_enable)
            m_log->unindent();
    }

    IndentScope(ILogger* log, Level level)
        : m_log(log)
        , m_enable(log->level() <= level)
    {
        ErAssert(log);

        if (m_enable)
            log->indent();
    }

    template <class... Args>
    IndentScope(ILogger* log, Level level, std::string_view format, Args&&... args)
        : m_log(log)
        , m_enable(log->level() <= level)
    {
        ErAssert(log);
        
        if (m_enable)
        {
            log->write(Record::make(
                level,
                System::PackedTime::now(),
                System::CurrentThread::id(),
                Format::vformat(format, Format::make_format_args(args...))
            ));

            log->indent();
        }
    }

private:
    ILogger* m_log;
    bool m_enable;
};


inline void writeln(ILogger* sink, Level level, const std::string& text)
{
    sink->write(Record::make(
        level,
        System::PackedTime::now(),
        System::CurrentThread::id(),
        text
    ));
}

inline void writeln(ILogger* sink, Level level, std::string&& text)
{
    sink->write(Record::make(
        level,
        System::PackedTime::now(),
        System::CurrentThread::id(),
        std::move(text)
    ));
}

template <class... Args>
void write(ILogger* sink, Level level, std::string_view format, Args&&... args)
{
    sink->write(Record::make(
        level, 
        System::PackedTime::now(), 
        System::CurrentThread::id(), 
        Format::vformat(format, Format::make_format_args(args...))
    ));
}

template <class... Args>
void debug(ILogger* sink, std::string_view format, Args&&... args)
{
    write(sink, Level::Debug, format, std::forward<Args>(args)...);
}

template <class... Args>
void info(ILogger* sink, std::string_view format, Args&&... args)
{
    write(sink, Level::Info, format, std::forward<Args>(args)...);
}

template <class... Args>
void warning(ILogger* sink, std::string_view format, Args&&... args)
{
    write(sink, Level::Warning, format, std::forward<Args>(args)...);
}

template <class... Args>
void error(ILogger* sink, std::string_view format, Args&&... args)
{
    write(sink, Level::Error, format, std::forward<Args>(args)...);
}

template <class... Args>
void fatal(ILogger* sink, std::string_view format, Args&&... args)
{
    write(sink, Level::Fatal, format, std::forward<Args>(args)...);
}


extern ER_RTL_EXPORT ILogger* g_global;
extern ER_RTL_EXPORT bool g_verbose;

inline ILogger* get() noexcept
{
    return g_global;
}

inline bool verbose() noexcept
{
    return g_verbose;
}

ER_RTL_EXPORT ILogger::Ptr global() noexcept;

} // namespace Er::Log {}

namespace Erp::Log
{

//
// not thread-safe
//

ER_RTL_EXPORT void setGlobal(Er::Log::ILogger::Ptr log) noexcept;

} // namespace Erp::Log {}


#define ErLogDebug(format, ...) \
    if (::Er::Log::get()->level() <= ::Er::Log::Level::Debug) \
        ::Er::Log::debug(::Er::Log::get(), format, ##__VA_ARGS__)

#define ErLogDebug2(sink, format, ...) \
    if (sink->level() <= ::Er::Log::Level::Debug) \
        ::Er::Log::debug(sink, format, ##__VA_ARGS__)


#define ErLogInfo(format, ...) \
    if (::Er::Log::get()->level() <= ::Er::Log::Level::Info) \
        ::Er::Log::info(::Er::Log::get(), format, ##__VA_ARGS__)

#define ErLogInfo2(sink, format, ...) \
    if (sink->level() <= ::Er::Log::Level::Info) \
        ::Er::Log::info(sink, format, ##__VA_ARGS__)


#define ErLogWarning(format, ...) \
    if (::Er::Log::get()->level() <= ::Er::Log::Level::Warning) \
        ::Er::Log::warning(::Er::Log::get(), format, ##__VA_ARGS__)

#define ErLogWarning2(sink, format, ...) \
    if (sink->level() <= ::Er::Log::Level::Warning) \
        ::Er::Log::warning(sink, format, ##__VA_ARGS__)


#define ErLogError(format, ...) \
    if (::Er::Log::get()->level() <= ::Er::Log::Level::Error) \
        ::Er::Log::error(::Er::Log::get(), format, ##__VA_ARGS__)

#define ErLogError2(sink, format, ...) \
    if (sink->level() <= ::Er::Log::Level::Error) \
        ::Er::Log::error(sink, format, ##__VA_ARGS__)


#define ErLogFatal(format, ...) \
    if (::Er::Log::get()->level() <= ::Er::Log::Level::Fatal) \
        ::Er::Log::fatal(::Er::Log::get(), format, ##__VA_ARGS__)

#define ErLogFatal2(sink, format, ...) \
    if (sink->level() <= ::Er::Log::Level::Fatal) \
        ::Er::Log::fatal(sink, format, ##__VA_ARGS__)



#define ErLogIndent(l, format, ...) \
    if (::Er::Log::get()->level() <= l) \
        ::Er::Log::write(::Er::Log::get(), l, format, ##__VA_ARGS__); \
    ::Er::Log::IndentScope __ids(::Er::Log::get(), l)

#define ErLogIndent2(sink, l, format, ...) \
    if (sink->level() <= l) \
        ::Er::Log::write(sink, l, format, ##__VA_ARGS__); \
    ::Er::Log::IndentScope __ids(sink, l)

