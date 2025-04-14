#pragma once

#include <erebus/rtl/property_bag.hxx>
#include <erebus/server/server_lib.hxx>

namespace Er
{

namespace SystemInfo
{

constexpr std::string_view ServerVersion{ "erebus/server_version" };

constexpr std::string_view OsType{ "system_info/os_type" };
constexpr std::string_view OsVersion{ "system_info/os_version" };


ER_SERVER_EXPORT [[nodiscard]] PropertyBag get(std::string_view name);


} // namespace SystemInfo {}

} // namespace Er {}