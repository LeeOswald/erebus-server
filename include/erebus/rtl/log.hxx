#pragma once

#include <erebus/iunknown.hxx>
#include <erebus/rtl/format.hxx>
#include <erebus/rtl/time.hxx>
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


struct IRecord
    : public IShared
{
    static constexpr std::string_view IID = "Er.Log.IRecord";

    [[nodiscard]] virtual Level level() const noexcept = 0;
    [[nodiscard]] virtual Time::ValueType time() const noexcept = 0;
    [[nodiscard]] virtual std::uintptr_t tid() const noexcept = 0;
    [[nodiscard]] virtual std::string_view component() const noexcept = 0;
    [[nodiscard]] virtual const std::string& message() const noexcept = 0;
    [[nodiscard]] virtual std::uint32_t indent() const noexcept = 0;

    // use in the top level logger only
    // since the structure may be shared between many consumers
    virtual void setComponent(std::string_view component) noexcept = 0;
    virtual void setIndent(std::uint32_t indent) noexcept = 0;

protected:
    virtual ~IRecord() = default;
};

using RecordPtr = SharedPtr<IRecord>;

ER_RTL_EXPORT [[nodiscard]] RecordPtr makeRecord(Level level, Time::ValueType time, uintptr_t tid, const std::string& message);
ER_RTL_EXPORT [[nodiscard]] RecordPtr makeRecord(Level level, Time::ValueType time, uintptr_t tid, std::string&& message);


struct IAtomicRecord // not thread-safe
    : public IDisposable
{
    static constexpr std::string_view IID = "Er.Log.IAtomicRecord";

    virtual bool empty() const noexcept = 0;
    virtual void push(RecordPtr r) = 0;
    virtual RecordPtr pop() = 0;
    virtual IAtomicRecord* clone() const = 0;

protected:
    virtual ~IAtomicRecord() = default;
};

using AtomicRecordPtr = DisposablePtr<IAtomicRecord>;

ER_RTL_EXPORT [[nodiscard]] AtomicRecordPtr makeAtomicRecord();


struct IFormatter
    : IDisposable
{
    static constexpr std::string_view IID = "Er.Log.IFormatter";

    [[nodiscard]] virtual std::string format(const IRecord* r) const = 0;

protected:
    virtual ~IFormatter() = default;
};

using FormatterPtr = DisposablePtr<IFormatter>;


using Filter = std::function<bool(const IRecord*)>;


struct ISink
{
    using Ptr = std::shared_ptr<ISink>;

    virtual ~ISink() = default;

    virtual void write(RecordPtr r) = 0;
    virtual void write(AtomicRecordPtr&& a) = 0;
    virtual void flush() = 0;
};


struct SinkBase
    : public ISink
    , public boost::noncopyable
{
    SinkBase(FormatterPtr&& formatter, auto&& filter) noexcept
        : m_formatter(std::move(formatter))
        , m_filter(std::forward<decltype(filter)>(filter))
    {}

    bool filter(const IRecord* r) const 
    {
        if (m_filter && !m_filter(r))
            return false;

        return true;
    }

    std::string format(const IRecord* r) const
    {
        return m_formatter->format(r);
    }

private:
    FormatterPtr m_formatter;
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
            log->write(makeRecord(
                level,
                Time::now(),
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
    sink->write(makeRecord(
        level,
        Time::now(),
        System::CurrentThread::id(),
        text
    ));
}

inline void writeln(ILogger* sink, Level level, std::string&& text)
{
    sink->write(makeRecord(
        level,
        Time::now(),
        System::CurrentThread::id(),
        std::move(text)
    ));
}

template <class... Args>
void write(ILogger* sink, Level level, std::string_view format, Args&&... args)
{
    sink->write(makeRecord(
        level, 
        Time::now(), 
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

