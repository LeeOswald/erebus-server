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

    virtual Level level() const noexcept = 0;
    virtual void write(Record::Ptr r) = 0;
    virtual void flush() = 0;
    virtual void indent() = 0;
    virtual void unindent() = 0;
};


struct ILog
    : public ISink
{
    virtual void addSink(ISink::Ptr sink) = 0;
    virtual void removeSink(ISink* sink) = 0;
};


struct Indent
    : public NonCopyable
{
    ~Indent()
    {
        sink->unindent();
    }

    Indent(ISink* sink)
        : sink(sink)
    {
        ErAssert(sink);
        sink->indent();
    }

private:
    ISink* sink;
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

} // namespace Er::Log2 {}