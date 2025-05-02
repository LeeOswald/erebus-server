#include "null_logger.hxx"



static Er::Log::LoggerPtr s_global;

static Er::Log::ILogger* null()
{
    static Er::Log::LoggerPtr _null = Er::Log::LoggerPtr{ new Er::Log::Private::NullLogger() };
    return _null.get();
}

static Er::Log::ILogger* s_null = null();

namespace Er::Log
{

ER_RTL_EXPORT ILogger* g_global = null();
ER_RTL_EXPORT bool g_verbose = false;


ER_RTL_EXPORT LoggerPtr global() noexcept
{
    return s_global;
}

} // namespace Er::Log {}


namespace Erp::Log
{

ER_RTL_EXPORT void setGlobal(Er::Log::LoggerPtr log) noexcept
{
    bool drainPending = (Er::Log::g_global == s_null) && !!log;

    s_global = log;
    Er::Log::g_global = log ? log.get() : s_null;

    // drain what has been captured by NullLogger
    if (drainPending)
    {
        auto _null = static_cast<Er::Log::Private::NullLogger*>(s_null);

        auto r = _null->pop();
        while (r)
        {
            Er::Log::g_global->write(r);
            r = _null->pop();
        }
    }
}

} // namespace Erp::Log {}