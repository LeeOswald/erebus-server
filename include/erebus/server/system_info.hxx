#pragma once

#include <erebus/rtl/property_bag.hxx>
#include <erebus/server/server_lib.hxx>

#include <functional>

namespace Er
{

namespace SystemInfo
{

constexpr std::string_view ServerVersion{ "erebus/server_version" };

constexpr std::string_view OsType{ "system_info/os_type" };
constexpr std::string_view OsVersion{ "system_info/os_version" };


} // namespace SystemInfo {}

namespace Server
{

namespace SystemInfo
{

using Source = std::function<Property(std::string_view)>;

ER_SERVER_EXPORT [[nodiscard]] PropertyBag get(std::string_view name);
ER_SERVER_EXPORT void registerSource(std::string_view name, Source&& src);


} // namespace SystemInfo {}

} // namespace Server {}

} // namespace Er {}