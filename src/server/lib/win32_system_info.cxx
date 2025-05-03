#include "erebus-version.h"

#include "system_info_common.hxx"

#include <erebus/rtl/format.hxx>
#include <erebus/rtl/system/unwindows.h>


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
    return Property{ Er::SystemInfo::OsType, std::string{"Windows"} };
}

Property osVersion(std::string_view)
{
    RTL_OSVERSIONINFOEXW versionInfo = {};
    if (NT_SUCCESS(RtlGetVersion(&versionInfo)))
    {
        return Property{ Er::SystemInfo::OsVersion, Er::format("{}.{}.{}", versionInfo.dwMajorVersion, versionInfo.dwMinorVersion, versionInfo.dwBuildNumber) };
    }

    return Property{ Er::SystemInfo::OsVersion , std::string("n/a")};
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
