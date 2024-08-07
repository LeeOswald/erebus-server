#pragma once

#include <erebus/knownprops.hxx>
#include <erebus/propertybag.hxx>
#include <erebus/util/exceptionutil.hxx>

#include <erebus-clt/erebus-clt.hxx>

#include "log.hxx"


extern std::optional<int> g_signalReceived;


inline void dumpPropertyBag(const Er::PropertyBag& bag, Er::Log::ILog* log)
{
    Er::enumerateProperties(bag, [log](const Er::Property& prop)
    {
        auto propInfo = Er::lookupProperty(prop.id).get();
        if (!propInfo)
        {
            log->writef(Er::Log::Level::Warning, "0x%08x: ???", prop.id);
        }
        else
        {
            std::ostringstream ss;
            propInfo->format(prop, ss);

            log->writef(Er::Log::Level::Info, "%s: %s", propInfo->name(), ss.str().c_str());
        }
    });
}
