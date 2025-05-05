#include "null_logger.hxx"



static Er::Log::LoggerPtr s_global;
static Er::Log::Private::NullLogger s_null;

namespace Er::Log
{

ER_RTL_EXPORT ILogger* g_global = &s_null;
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
    bool drainPending = (Er::Log::g_global == &s_null) && !!log;

    s_global = log;
    if (log)
        Er::Log::g_global = log.get();
    else 
        Er::Log::g_global = &s_null;

    // drain what has been captured by NullLogger
    if (drainPending)
    {
        auto _null = static_cast<Er::Log::Private::NullLogger*>(&s_null);

        auto r = _null->pop();
        while (r)
        {
            Er::Log::g_global->write(r);
            r = _null->pop();
        }
    }
}

} // namespace Erp::Log {}