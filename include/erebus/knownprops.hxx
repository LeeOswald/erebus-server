#pragma once

#include <erebus/property.hxx>


namespace Er
{

namespace Private
{

EREBUS_EXPORT void initializeKnownProps();
EREBUS_EXPORT void finalizeKnownProps();

} // namespace Private {}


EREBUS_EXPORT void registerProperty(IPropertyInfo::Ptr pi);
EREBUS_EXPORT void unregisterProperty(IPropertyInfo::Ptr pi) noexcept;
EREBUS_EXPORT IPropertyInfo::Ptr lookupProperty(PropId id) noexcept;
EREBUS_EXPORT IPropertyInfo::Ptr lookupProperty(const char* id) noexcept;

inline IPropertyInfo* getPropertyInfo(const Property& prop) noexcept
{
    if (prop.info)
        return prop.info;

    auto info = lookupProperty(prop.id);
    if (!info)
        return nullptr;
    
    return info.get();
}

inline IPropertyInfo* cachePropertyInfo(const Property& prop) noexcept
{
    if (prop.info)
        return prop.info;

    auto info = lookupProperty(prop.id);
    if (!info)
        return nullptr;

    prop.info = info.get();
    return prop.info;
}

inline void cachePropertyInfo(const PropertyBag& bag) noexcept
{
    for (auto& prop: bag)
    {
        cachePropertyInfo(prop.second);
    }
}

} // namespace Er {}
