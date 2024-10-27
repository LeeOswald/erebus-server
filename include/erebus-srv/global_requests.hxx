#pragma once

#include <erebus/knownprops.hxx>

namespace Er::Server
{

namespace Props
{

constexpr const std::string_view Domain = "server";

using SystemName = PropertyValue<std::string, ER_PROPID("system_name"), "System name">;
using ServerVersion = PropertyValue<std::string, ER_PROPID("server_version"), "Server version">;

namespace Private
{

inline void registerAll(Er::Log::ILog* log)
{
    registerProperty(Domain, SystemName::make_info(), log);
    registerProperty(Domain, ServerVersion::make_info(), log);
}

inline void unregisterAll(Er::Log::ILog* log)
{
    unregisterProperty(Domain, lookupProperty(Domain, SystemName::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, ServerVersion::Id::value), log);
}

} // namespace Private {}

} // namespace Props {}

namespace Requests
{

static const std::string_view GetVersion = "get_version";

} // namespace Requests {}


} // namespace Er::Server {}