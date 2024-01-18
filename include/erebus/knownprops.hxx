#pragma once

#include <erebus/property.hxx>


namespace Er
{

EREBUS_EXPORT void registerProperty(IPropertyInfo* pi);
EREBUS_EXPORT IPropertyInfo* lookupProperty(PropId id);
EREBUS_EXPORT IPropertyInfo* lookupProperty(const char* id);

} // namespace Er {}
