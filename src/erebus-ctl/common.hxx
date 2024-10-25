#pragma once

#include <erebus/knownprops.hxx>
#include <erebus/propertybag.hxx>
#include <erebus/util/exceptionutil.hxx>

#include <erebus-clt/erebus-clt.hxx>


extern std::optional<int> g_signalReceived;


inline void dumpPropertyBag(std::string_view domain, const Er::PropertyBag& bag, Er::Log::ILog* log)
{
    Er::enumerateProperties(bag, [log, domain](const Er::Property& prop)
    {
        auto propInfo = Er::lookupProperty(domain, prop.id).get();
        if (!propInfo)
        {
            Er::Log::write(log, Er::Log::Level::Warning, "{:08x}: {}", prop.id, prop.to_string());
        }
        else
        {
            Er::Log::write(log, Er::Log::Level::Info, "{}: {}", propInfo->name(), propInfo->to_string(prop));
        }
    });
}
