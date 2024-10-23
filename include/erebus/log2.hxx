#pragma once

#include <erebus/format.hxx>
#include <erebus/system/packed_time.hxx>
#include <erebus/system/thread.hxx>

namespace Er::Log2
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
{
    Level level = Level::Info;
    System::PackedTime::ValueType time;
    uintptr_t tid = 0;
    std::string message;
    unsigned indent = 0;

    using Ptr = std::shared_ptr<Record>;

    template <typename MessageT>
    static Ptr make(Level level, System::PackedTime::ValueType time, uintptr_t tid, MessageT&& message)
    {
        return std::make_shared<Record>(level, time, tid, std::forward<MessageT>(message));
    }

private:
    template <typename MessageT>
    explicit Record(Level level, System::PackedTime::ValueType time, uintptr_t tid, MessageT&& message)
        : level(level)
        , time(time)
        , tid(tid)
        , message(std::forward<MessageT>(message))
    {
    }
};


struct ISink
{
    using Ptr = std::shared_ptr<ISink>;

    virtual ~ISink() = default;
    
    constexpr ISink() noexcept = default;
    
    constexpr Level level() const noexcept
    {
        return m_level;
    }

    Level setLevel(Level level)
    {
        auto prev = m_level;
        m_level = level;
        doSetLevel(level);
        return prev;
    }

    virtual void write(Record::Ptr r) = 0;
    virtual void flush() = 0;
    
protected:
    virtual void doSetLevel(Level) { }

    Level m_level = Level::Debug;
};


struct IFilter
    : public ISink
{
    using Ptr = std::shared_ptr<IFilter>;

    virtual void addSink(std::string_view name, ISink::Ptr sink) = 0;
    virtual void removeSink(std::string_view name) = 0;
};


struct ILogger
    : public IFilter
{
    using Ptr = std::shared_ptr<ILogger>;

    virtual void indent() = 0;
    virtual void unindent() = 0;
};


struct Indent
    : public NonCopyable
{
    ~Indent()
    {
        log->unindent();
    }

    Indent(ILogger* log)
        : log(log)
    {
        ErAssert(log);
        log->indent();
    }

private:
    ILogger* log;
};


template <class... Args>
void write(ISink* sink, Level level, std::string_view format, Args&&... args)
{
    sink->write(Record::make(
        level, 
        System::PackedTime::now(), 
        System::CurrentThread::id(), 
        Format::vformat(format, Format::make_format_args(args...))
    ));
}

template <class... Args>
void debug(ISink* sink, std::string_view format, Args&&... args)
{
    if (sink->level() <= Level::Debug)
        write(sink, Level::Debug, format, std::forward<Args>(args...));
}

template <class... Args>
void info(ISink* sink, std::string_view format, Args&&... args)
{
    if (sink->level() <= Level::Info)
        write(sink, Level::Info, format, std::forward<Args>(args...));
}

template <class... Args>
void warning(ISink* sink, std::string_view format, Args&&... args)
{
    if (sink->level() <= Level::Warning)
    {
        write(sink, Level::Warning, format, std::forward<Args>(args...));
        sink->flush();
    }
}

template <class... Args>
void error(ISink* sink, std::string_view format, Args&&... args)
{
    if (sink->level() <= Level::Error)
    {
        write(sink, Level::Error, format, std::forward<Args>(args...));
        sink->flush();
    }
}

template <class... Args>
void fatal(ISink* sink, std::string_view format, Args&&... args)
{
    if (sink->level() <= Level::Fatal)
    {
        write(sink, Level::Fatal, format, std::forward<Args>(args...));
        sink->flush();
    }
}


enum class ThreadSafe
{
    No,
    Yes
};


EREBUS_EXPORT IFilter::Ptr makeTee(ThreadSafe mode);
EREBUS_EXPORT ILogger::Ptr makeAsyncLogger();


} // namespace Er::Log2 {}