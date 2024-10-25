#pragma once

#include <erebus/exception.hxx>
#include <erebus/log.hxx>

namespace Er
{

namespace Util
{

EREBUS_EXPORT std::string formatException(const std::exception& e) noexcept;
EREBUS_EXPORT std::string formatException(const Er::Exception& e) noexcept;

void EREBUS_EXPORT logException(Log::ILog* log, Log::Level level, const std::exception& e) noexcept;
void EREBUS_EXPORT logException(Log::ILog* log, Log::Level level, const Er::Exception& e) noexcept;


} // namespace Util {}


template <typename ResultT, typename WorkT, typename... Args>
ResultT protectedCall(Er::Log::ILog* log, WorkT work, Args&&... args)
{
    try
    {
        return work(std::forward<Args>(args)...);
    }
    catch (Er::Exception& e)
    {
        if (log)
            Util::logException(log, Er::Log::Level::Error, e);
    }
    catch (std::exception& e)
    {
        if (log)
            Util::logException(log, Er::Log::Level::Error, e);
    }
    catch (...)
    {
        if (log)
            Log::writeln(log, Er::Log::Level::Error, "Unexpected exception");
    }

    return ResultT();
}


} // namespace Er {}

