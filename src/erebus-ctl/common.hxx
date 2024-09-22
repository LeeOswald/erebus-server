#pragma once

#include <erebus/knownprops.hxx>
#include <erebus/propertybag.hxx>
#include <erebus/util/exceptionutil.hxx>

#include <erebus-clt/erebus-clt.hxx>

#include "log.hxx"


extern std::optional<int> g_signalReceived;


inline void dumpPropertyBag(std::string_view domain, const Er::PropertyBag& bag, Er::Log::ILog* log)
{
    Er::enumerateProperties(bag, [log, domain](const Er::Property& prop)
    {
        auto propInfo = Er::lookupProperty(domain, prop.id).get();
        if (!propInfo)
        {
            log->writef(Er::Log::Level::Warning, "0x%08x: %s", prop.id, prop.to_string().c_str());
        }
        else
        {
            log->writef(Er::Log::Level::Info, "%s: %s", propInfo->name(), propInfo->to_string(prop).c_str());
        }
    });
}
