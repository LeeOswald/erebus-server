#include <erebus/rtl/logger/syslog_linux_sink.hxx>

#include <syslog.h>



namespace Er::Log
{

namespace
{

class SyslogSink
    : public SinkBase
{
public:
    ~SyslogSink()
    {
        ::closelog();
    }

    SyslogSink(const char* tag, IFormatter::Ptr formatter, Filter&& filter)
        : SinkBase(std::move(formatter), std::move(filter))
    {
        ::openlog(tag, LOG_CONS, LOG_DAEMON);
    }

    void write(Record::Ptr r) override
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

    void write(AtomicRecord a) override
    {
        for (auto r: a)
            write(r);
    }

    void flush() override
    {
    }
};


} // namespace {}


ER_RTL_EXPORT ISink::Ptr makeSyslogSink(const char* tag, IFormatter::Ptr formatter, Filter&& filter)
{
    return std::make_shared<SyslogSink>(tag, std::move(formatter), std::move(filter));
}


} // namespace Er::Log {}