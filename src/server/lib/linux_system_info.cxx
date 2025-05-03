#include "erebus-version.h"

#include "system_info_common.hxx"

#include <erebus/rtl/format.hxx>

#include <sys/utsname.h>


namespace Er::Server::SystemInfo::Private
{

namespace
{


Property serverVersion(std::string_view)
{
    return Property{ Er::SystemInfo::ServerVersion, Er::format("{}.{}.{}", ER_VERSION_MAJOR, ER_VERSION_MINOR, ER_VERSION_PATCH) };
}

Property osType(std::string_view)
{
    struct utsname u = {};
    if (::uname(&u) == 0)
    {
        return Property{ Er::SystemInfo::OsType, std::string{u.sysname} };
    }

    return {};
}

Property osVersion(std::string_view)
{
    struct utsname u = {};
    if (::uname(&u) == 0)
    {
        return Property{ Er::SystemInfo::OsType, std::string{u.release} };
    }

    return {};
}

} // namespace {}


void registerSources(Sources& s)
{
    std::unique_lock l(s.mutex);

    s.map.insert({ Er::SystemInfo::ServerVersion, { serverVersion } });
    s.map.insert({ Er::SystemInfo::OsType, { osType } });
    s.map.insert({ Er::SystemInfo::OsVersion, { osVersion } });
}

} // namespace Er::Server::SystemInfo::Private {}
