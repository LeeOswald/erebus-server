#pragma once

#include <erebus/knownprops.hxx>
#include <erebus/propertybag.hxx>
#include <erebus/util/exceptionutil.hxx>

#include <erebus-clt/erebus-clt.hxx>


extern std::optional<int> g_signalReceived;

inline std::string propToString(const Er::Property& prop)
{
    if (prop.type() == Er::PropertyType::Binary)
    {
        return Er::Format::format("<binary ({} bytes)>", prop.value.getBinary().size());
    }
    else if (prop.type() == Er::PropertyType::Binaries)
    {
        return Er::Format::format("<binary[{}]>", prop.value.getBinaries().size());
    }

    return prop.to_string();
}

inline std::string propToString(const Er::Property& prop, Er::IPropertyInfo* info)
{
    if ((prop.type() == Er::PropertyType::Binary) || (prop.type() == Er::PropertyType::Binaries))
    {
        return propToString(prop);
    }

    return info->to_string(prop);
}


inline void dumpPropertyBag(std::string_view domain, const Er::PropertyBag& bag, Er::Log::ILog* log)
{
    Er::enumerateProperties(bag, [log, domain](const Er::Property& prop)
    {
        auto propInfo = Er::lookupProperty(domain, prop.id).get();
        if (!propInfo)
        {
            Er::Log::write(log, Er::Log::Level::Warning, "{:08x}: {}", prop.id, propToString(prop));
        }
        else
        {
            Er::Log::write(log, Er::Log::Level::Info, "{}: {}", propInfo->name(), propToString(prop, propInfo));
        }
    });
}
