#pragma once


#include <erebus-desktop/erebus-desktop.hxx>
#include <erebus/util/inifile.hxx>

#include <boost/iostreams/device/mapped_file.hpp>


namespace Er
{

namespace Desktop
{

namespace Private
{


struct DesktopEntry
{
    std::string name;
    std::string exec;
    std::string icon;
};


template <typename PathT>
DesktopEntry loadDesktopFile(const PathT& path)
{
    boost::iostreams::mapped_file_source file(path);

    auto ini = Er::Util::IniFile::parse(std::string_view(file.data(), file.size()));

    DesktopEntry e;
    auto name = Er::Util::IniFile::lookup(ini, std::string_view("Desktop Entry"), std::string_view("Name"));
    if (name)
        e.name = *name;

    auto exec = Er::Util::IniFile::lookup(ini, std::string_view("Desktop Entry"), std::string_view("Exec"));
    if (exec)
        e.exec = *exec;

    auto icon = Er::Util::IniFile::lookup(ini, std::string_view("Desktop Entry"), std::string_view("Icon"));
    if (icon)
        e.icon = *icon;

    return e;
}


std::string dekstopFilePathForPid(uint64_t pid);


} // namespace Private {}

} // namespace Desktop {}

} // namespace Er {}