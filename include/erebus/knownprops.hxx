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


EREBUS_EXPORT void registerProperty(std::string_view domain, IPropertyInfo::Ptr pi, Er::Log::ILog* log);
EREBUS_EXPORT void unregisterProperty(std::string_view domain, IPropertyInfo::Ptr pi, Er::Log::ILog* log) noexcept;
EREBUS_EXPORT IPropertyInfo::Ptr lookupProperty(std::string_view domain, PropId id) noexcept;
EREBUS_EXPORT IPropertyInfo::Ptr lookupProperty(std::string_view domain, const char* id) noexcept;


} // namespace Er {}
