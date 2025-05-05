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


struct ER_RTL_EXPORT Record final
{
private:
    struct PrivateTag {};

public:
    ~Record() = default;

    Record(PrivateTag, Level level, Time::ValueType time, uintptr_t tid, auto&& message)
        : m_component()
        , m_level(level)
        , m_time(time)
        , m_tid(tid)
        , m_message(std::forward<decltype(message)>(message))
        , m_indent(0)
    {
    }

    [[nodiscard]] static std::shared_ptr<Record> make(Level level, Time::ValueType time, uintptr_t tid, auto&& message)
    {
        return std::make_shared<Record>(PrivateTag{}, level, time, tid, std::forward<decltype(message)>(message));
    }

    [[nodiscard]] Level level() const noexcept
    {
        return m_level;
    }

    [[nodiscard]] Time::ValueType time() const noexcept
    {
        return m_time;
    }

    [[nodiscard]] std::uintptr_t tid() const noexcept
    {
        return m_tid;
    }

    [[nodiscard]] std::string_view component() const noexcept
    {
        return m_component;
    }

    [[nodiscard]] const std::string& message() const noexcept
    {
        return m_message;
    }

    [[nodiscard]] std::uint32_t indent() const noexcept
    {
        return m_indent;
    }

    // use the following methods in top logger only 
    // (i.e. while there's no other owners referencing this record)
    void setComponent(std::string_view component) noexcept
    {
        m_component = component;
    }

    void setIndent(std::uint32_t indent) noexcept
    {
        m_indent = indent;
    }

private:
    std::string_view m_component;
    const Level m_level;
    const Time::ValueType m_time;
    const uintptr_t m_tid;
    const std::string m_message;
    std::uint32_t m_indent;
};

using RecordPtr = std::shared_ptr<Record>;


struct ER_RTL_EXPORT AtomicRecord final
{
private:
    struct PrivateTag {};

public:
    ~AtomicRecord() = default;

    explicit AtomicRecord(PrivateTag, std::vector<RecordPtr>&& records) noexcept
        : m_records(std::move(records))
    {
    }

    [[nodiscard]] static std::shared_ptr<AtomicRecord> make(std::vector<RecordPtr>&& records) noexcept
    {
        return std::make_shared<AtomicRecord>(PrivateTag{}, std::move(records));
    }

    const auto& get() const noexcept
    {
        return m_records;
    }

private:
    std::vector<RecordPtr> m_records;
};

using AtomicRecordPtr = std::shared_ptr<AtomicRecord>;


struct IFormatter
    : public IReferenceCounted
{
    static constexpr std::string_view IID = "Er.Log.IFormatter";

    [[nodiscard]] virtual std::string format(const Record* r) const = 0;

protected:
    virtual ~IFormatter() = default;
};

using FormatterPtr = ReferenceCountedPtr<IFormatter>;


struct IFilter
    : public IReferenceCounted
{
    static constexpr std::string_view IID = "Er.Log.IFilter";

    [[nodiscard]] virtual bool filter(const Record* r) const noexcept = 0;

protected:
    virtual ~IFilter() = default;
};

using FilterPtr = ReferenceCountedPtr<IFilter>;


struct ISink
    : public IReferenceCounted
{
    static constexpr std::string_view IID = "Er.Log.ISink";

    virtual void write(RecordPtr r) = 0;
    virtual void write(AtomicRecordPtr a) = 0;
    virtual void flush() = 0;

protected:
    virtual ~ISink() = default;
};

using SinkPtr = ReferenceCountedPtr<ISink>;


struct ITee
    : public ISink
{
    static constexpr std::string_view IID = "Er.Log.ITee";

    virtual void addSink(std::string_view name, SinkPtr sink) = 0;
    virtual void removeSink(std::string_view name) = 0;
    virtual SinkPtr findSink(std::string_view name) = 0;

protected:
    virtual ~ITee() = default;
};

using TeePtr = ReferenceCountedPtr<ITee>;

ER_RTL_EXPORT TeePtr makeTee(ThreadSafe mode);


struct ILogger
    : public ITee
{
    static constexpr std::string_view IID = "Er.Log.ILogger";

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
    virtual ~ILogger() = default;

    Level m_level = Level::Debug;
};

using LoggerPtr = ReferenceCountedPtr<ILogger>;

ER_RTL_EXPORT LoggerPtr makeLogger(std::string_view component = {}, std::chrono::milliseconds threshold = {});
ER_RTL_EXPORT LoggerPtr makeSyncLogger(std::string_view component = {});


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
            log->write(
                level,
                Time::now(),
                System::CurrentThread::id(),
                Format::vformat(format, Format::make_format_args(args...))
            );

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
        Time::now(),
        System::CurrentThread::id(),
        text
    ));
}

inline void writeln(ILogger* sink, Level level, std::string&& text)
{
    sink->write(Record::make(
        level,
        Time::now(),
        System::CurrentThread::id(),
        std::move(text)
    ));
}

template <class... Args>
void write(ILogger* sink, Level level, std::string_view format, Args&&... args)
{
    sink->write(Record::make(
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

ER_RTL_EXPORT LoggerPtr global() noexcept;

} // namespace Er::Log {}

namespace Erp::Log
{

//
// not thread-safe
//

ER_RTL_EXPORT void setGlobal(Er::Log::LoggerPtr log) noexcept;

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

