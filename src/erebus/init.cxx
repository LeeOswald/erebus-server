#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>
#include <erebus/log.hxx>
#include <erebus/luaxx.hxx>

#if ER_WINDOWS
#include <erebus/util/utf16.hxx>
#endif

#include <atomic>
#include <syncstream>

namespace Er
{

namespace
{

class DummyLogger
    : public Log::ILog
    , public NonCopyable
{
public:
    ~DummyLogger() = default;
    DummyLogger() = default;

    void write(Log::Record::Ptr r) override
    {
        if (r->level() < Log::Level::Error)
            std::osyncstream(std::cerr) << r->message() << std::endl;
        else
            std::osyncstream(std::cout) << r->message() << std::endl;

#if ER_WINDOWS
        if (::IsDebuggerPresent())
        {
            auto u16 = Util::utf8ToUtf16(r->message());
            u16.append(L"\n");
            ::OutputDebugStringW(u16.c_str());
        }
#endif
    }

    void flush() override
    {
    }

    void addSink(std::string_view, ISink::Ptr) override
    {
    }

    void removeSink(std::string_view) override
    {
    }

    void indent() override
    {
    }

    void unindent() override
    {
    }
};


std::atomic<long> g_initialized = 0;

DummyLogger s_dummyLogger;

Log::ILog* s_log = &s_dummyLogger;

} // namespace {}

namespace Log
{

EREBUS_EXPORT Log::ILog* defaultLog() noexcept
{
    return s_log;
}

} // namespace Log {}

EREBUS_EXPORT void initialize(Er::Log::ILog* log)
{
    if (g_initialized.fetch_add(1, std::memory_order_acq_rel) == 0)
    {
        s_log = log;

        Er::Private::initializeKnownProps();

        Er::ExceptionProps::Private::registerAll(log);

        Er::initializeLua(log);
    }
}

EREBUS_EXPORT void finalize(Er::Log::ILog* log) noexcept
{
    if (g_initialized.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        Er::finalizeLua();

        Er::ExceptionProps::Private::unregisterAll(log);
        
        Er::Private::finalizeKnownProps();

        s_log = &s_dummyLogger;
    }
}

} // namespace Er {}
