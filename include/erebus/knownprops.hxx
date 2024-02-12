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
    auto info = lookupProperty(prop.id);
    assert(info);
    return info.get();
}

} // namespace Er {}
