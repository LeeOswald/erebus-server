#pragma once

#include <erebus/rtl/format.hxx>
#include <erebus/server/system_info.hxx>

#include <functional>
#include <map>


namespace Er
{

namespace SystemInfo
{

namespace Private
{

using SystemInfoSource = std::function<Property(std::string_view)>;
using SystemInfoSources = std::map<std::string_view, SystemInfoSource>;


SystemInfoSources registerSources();
std::map<std::string_view, SystemInfoSource> const& sources();

} // namespace Private {}

} // namespace SystemInfo {}

} // namespace Er {}