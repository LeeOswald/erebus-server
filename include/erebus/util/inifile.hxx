#pragma once

#include <erebus/erebus.hxx>

#include <map>


namespace Er
{

namespace Util
{

namespace IniFile
{


using Key = std::string_view;
using Value = std::string_view;
using Section = std::map<Key, Value>;
using Sections = std::map<Key, Section>;


EREBUS_EXPORT Sections parse(std::string_view raw);
EREBUS_EXPORT std::optional<std::string_view> lookup(const Sections& ini, std::string_view section, std::string_view key);


} // namespace IniFile {}

} // namespace Util {}

} // namespace Er {]