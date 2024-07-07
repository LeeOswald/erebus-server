#pragma once

#include <erebus/log.hxx>
#include <erebus/propertybag.hxx>


namespace Er
{

namespace Private
{

EREBUS_EXPORT void initializeKnownProps();
EREBUS_EXPORT void finalizeKnownProps();

} // namespace Private {}


EREBUS_EXPORT void registerProperty(IPropertyInfo::Ptr pi, Er::Log::ILog* log);
EREBUS_EXPORT void unregisterProperty(IPropertyInfo::Ptr pi, Er::Log::ILog* log) noexcept;
EREBUS_EXPORT IPropertyInfo::Ptr lookupProperty(PropId id) noexcept;
EREBUS_EXPORT IPropertyInfo::Ptr lookupProperty(const char* id) noexcept;

inline IPropertyInfo* getPropertyInfo(const Property& prop) noexcept
{
    if (prop.info)
        return prop.info.get();

    auto info = lookupProperty(prop.id);
    if (!info)
        return nullptr;
    
    return info.get();
}

inline IPropertyInfo* cachePropertyInfo(const Property& prop) noexcept
{
    if (prop.info)
        return prop.info.get();

    auto info = lookupProperty(prop.id);
    if (!info)
        return nullptr;

    prop.info = info;
    return prop.info.get();
}

inline void cachePropertyInfo(const PropertyMap& bag) noexcept
{
    for (auto& prop: bag)
    {
        cachePropertyInfo(prop.second);
    }
}

inline void cachePropertyInfo(const PropertyVector& bag) noexcept
{
    for (auto& prop: bag)
    {
        cachePropertyInfo(prop);
    }
}


} // namespace Er {}
