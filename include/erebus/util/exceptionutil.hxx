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
ResultT protectedCall(Er::Log::ILog* log, const Er::Log::Location& location, WorkT work, Args&&... args)
{
    try
    {
        return work(std::forward<Args>(args)...);
    }
    catch (Er::Exception& e)
    {
        if (log)
        {
            auto msg = Er::Util::formatException(e);
            log->write(Er::Log::Level::Error, location, "%s", msg.c_str());
        }
    }
    catch (std::exception& e)
    {
        if (log)
        {
            auto msg = Er::Util::formatException(e);
            log->write(Er::Log::Level::Error, location, "%s", msg.c_str());
        }
    }
    catch (...)
    {
        if (log)
            log->write(Er::Log::Level::Error, location, "Unexpected exception");
    }

    return ResultT();
}


} // namespace Er {}

