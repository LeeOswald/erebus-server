#pragma once

#include <erebus/knownprops.hxx>

namespace Er::Server
{

namespace Props
{

constexpr const std::string_view Domain = "server";

using SystemName = PropertyValue<std::string, ER_PROPID("system_name"), "System name">;
using ServerVersion = PropertyValue<std::string, ER_PROPID("server_version"), "Server version">;

using PingData = PropertyValue<Binary, ER_PROPID("ping_data"), "Ping data">;
using PingDataSize = PropertyValue<uint32_t, ER_PROPID("ping_data_size"), "Ping data size">;
using PingSequence = PropertyValue<uint32_t, ER_PROPID("ping_sequence"), "Ping sequence">;
using PingSender = PropertyValue<uint32_t, ER_PROPID("ping_sender"), "Ping sender ID">;

namespace Private
{

inline void registerAll(Er::Log::ILog* log)
{
    registerProperty(Domain, SystemName::make_info(), log);
    registerProperty(Domain, ServerVersion::make_info(), log);
    registerProperty(Domain, PingData::make_info(), log);
    registerProperty(Domain, PingDataSize::make_info(), log);
    registerProperty(Domain, PingSequence::make_info(), log);
    registerProperty(Domain, PingSender::make_info(), log);
}

inline void unregisterAll(Er::Log::ILog* log)
{
    unregisterProperty(Domain, lookupProperty(Domain, SystemName::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, ServerVersion::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, PingData::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, PingDataSize::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, PingSequence::Id::value), log);
    unregisterProperty(Domain, lookupProperty(Domain, PingSender::Id::value), log);
}

} // namespace Private {}

} // namespace Props {}

namespace Requests
{

static const std::string_view GetVersion = "get_version";
static const std::string_view Ping = "ping";

} // namespace Requests {}


} // namespace Er::Server {}