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
EREBUS_EXPORT void unregisterProperty(IPropertyInfo::Ptr pi);
EREBUS_EXPORT IPropertyInfo::Ptr lookupProperty(PropId id);
EREBUS_EXPORT IPropertyInfo::Ptr lookupProperty(const char* id);

} // namespace Er {}
