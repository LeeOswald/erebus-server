#pragma once

#include <erebus/knownprops.hxx>
#include <erebus/propertybag.hxx>
#include <erebus/util/exceptionutil.hxx>

#include <erebus-clt/erebus-clt.hxx>

#include "log.hxx"


extern std::optional<int> g_signalReceived;


template <typename Work>
void protectedCall(Er::Log::ILog* log, Work w) noexcept
{
    try
    {
        w();
    }
    catch (Er::Exception& e)
    {
        Er::Util::logException(log, Er::Log::Level::Error, e);
    }
    catch (std::exception& e)
    {
        Er::Util::logException(log, Er::Log::Level::Error, e);
    }
}

inline void dumpPropertyBag(const Er::PropertyBag& info, Er::Log::ILog* log)
{
    for (auto it = info.begin(); it != info.end(); ++it)
    {
        auto propInfo = Er::lookupProperty(it->second.id).get();
        if (!propInfo)
        {
            log->write(Er::Log::Level::Warning, ErLogNowhere(), "0x%08x: ???", it->second.id);
        }
        else
        {
            std::ostringstream ss;
            propInfo->format(it->second, ss);

            log->write(Er::Log::Level::Info, ErLogNowhere(), "%s: %s", propInfo->name(), ss.str().c_str());
        }
    }
}
