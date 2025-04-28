#include <erebus/rtl/logger/null_logger.hxx>



static Er::Log::NullLogger s_null;
static Er::Log::ILogger::Ptr s_global;

namespace Er::Log
{

ER_RTL_EXPORT ILogger* g_global = &s_null;
ER_RTL_EXPORT bool g_verbose = false;


ER_RTL_EXPORT ILogger::Ptr global() noexcept
{
    return s_global;
}

} // namespace Er::Log {}


namespace Erp::Log
{

ER_RTL_EXPORT void setGlobal(Er::Log::ILogger::Ptr log) noexcept
{
    bool drainPending = (Er::Log::g_global == &s_null) && !!log;

    s_global = log;
    Er::Log::g_global = log ? log.get() : &s_null;

    if (drainPending)
    {
        auto r = s_null.pop();
        while (r)
        {
            Er::Log::g_global->write(r);
            r = s_null.pop();
        }
    }
}

} // namespace Erp::Log {}