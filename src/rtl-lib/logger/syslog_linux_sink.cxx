#include <erebus/rtl/logger/syslog_linux_sink.hxx>

#include <syslog.h>

#include "sink_base.hxx"


namespace Er::Log
{

namespace
{

class SyslogSink
    : public Private::SinkBase
{
public:
    ~SyslogSink()
    {
        ::closelog();
    }

    SyslogSink(const char* tag, FormatterPtr&& formatter, FilterPtr&& filter)
        : Private::SinkBase(std::move(formatter), std::move(filter))
    {
        ::openlog(tag, LOG_CONS, LOG_DAEMON);
    }

    void write(Level level, Time::ValueType time, uintptr_t tid, const std::string& message) override
    {
        write(makeRecord({}, level, time, tid, message, 0));
    }

    void write(Level level, Time::ValueType time, uintptr_t tid, std::string&& message) override
    {
        write(makeRecord({}, level, time, tid, std::move(message), 0));
    }

    void write(RecordPtr r) override
    {
        if (!filter(r.get()))
            return;

        auto formatted = format(r.get());
        const auto available = formatted.length();
        if (!available)
            return;

        int priority;
        switch (r->level())
        {
        case Er::Log::Level::Debug: priority = LOG_DEBUG; break;
        case Er::Log::Level::Info: priority = LOG_INFO; break;
        case Er::Log::Level::Warning: priority = LOG_WARNING; break;
        case Er::Log::Level::Error: priority = LOG_ERR; break;
        default: priority = LOG_ERR; break;
        }

        ::syslog(priority, "%s", formatted.c_str());
    }

    void write(AtomicRecordPtr a) override
    {
        auto count = a->size();
        for (decltype(count) i = 0; i < count; ++i)
            write(a->get(i));
    }

    void flush() override
    {
    }
};


} // namespace {}


ER_RTL_EXPORT SinkPtr makeSyslogSink(const char* tag, FormatterPtr&& formatter, FilterPtr&& filter)
{
    return SinkPtr(new SyslogSink(tag, std::move(formatter), std::move(filter)));
}


} // namespace Er::Log {}