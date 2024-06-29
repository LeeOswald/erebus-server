#pragma once

#include <erebus/erebus.hxx>

#include <unordered_map>


namespace Er
{

namespace Util
{

namespace IniFile
{


using KeyView = std::string_view;
using ValueView = std::string_view;
using SectionView = std::unordered_map<KeyView, ValueView>;
using IniView = std::unordered_map<KeyView, SectionView>;

EREBUS_EXPORT IniView parse(std::string_view raw);


inline std::optional<std::string_view> lookup(const IniView& ini, std::string_view section, std::string_view key)
{
    auto secIt = ini.find(section);
    if (secIt == ini.end())
        return std::nullopt;

    auto valIt = secIt->second.find(key);
    if (valIt == secIt->second.end())
        return std::nullopt;

    return valIt->second;
}


} // namespace IniFile {}

} // namespace Util {}

} // namespace Er {]