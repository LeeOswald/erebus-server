#pragma once

#include <erebus/empty.hxx>
#include <erebus/exception.hxx>
#include <erebus/property.hxx>
#include <erebus/knownprops.hxx>

#include <erebus/erebus.pb.h>



namespace Er
{
    
namespace Protocol
{

void assignProperty(erebus::Property& out, const Property& source);
Property getProperty(const erebus::Property& source);

namespace Props
{

constexpr const std::string_view Domain = "server";

using RemoteSystemDesc = PropertyValue<std::string, ER_PROPID("system_description"), "Remote system">;
using ServerVersionString = PropertyValue<std::string, ER_PROPID("server_version"), "Server version">;

namespace Private
{

inline void registerAll(Er::Log::ILog* log)
{
    registerProperty(Domain, RemoteSystemDesc::make_info(), log);
    registerProperty(Domain, ServerVersionString::make_info(), log);
}

inline void unregisterAll(Er::Log::ILog* log)
{
    unregisterProperty(Domain, lookupProperty(Domain, RemoteSystemDesc::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, ServerVersionString::Id::value), log);
}

} // namespace Private {}

} // namespace Props {}

namespace GenericRequests
{

static const std::string_view GetVersion = "get_version";

} // namespace GenericRequests {}


} // namespace Protocol {}
    
} // namespace Er {}