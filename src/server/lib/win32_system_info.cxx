#include "erebus-version.h"

#include "system_info_common.hxx"

#include <erebus/rtl/format.hxx>
#include <erebus/rtl/system/unwindows.h>


namespace Er::SystemInfo::Private
{

namespace 
{


Property serverVersion(std::string_view)
{
    return Property{ ServerVersion, Er::format("{}.{}.{}", ER_VERSION_MAJOR, ER_VERSION_MINOR, ER_VERSION_PATCH) };
}

Property osType(std::string_view)
{
    return Property{ OsType, std::string{"Windows"} };
}

Property osVersion(std::string_view)
{
    RTL_OSVERSIONINFOEXW versionInfo = {};
    if (NT_SUCCESS(RtlGetVersion(&versionInfo)))
    {
        return Property{ OsVersion, Er::format("{}.{}.{}", versionInfo.dwMajorVersion, versionInfo.dwMinorVersion, versionInfo.dwBuildNumber) };
    }

    return Property{ OsVersion , std::string("n/a")};
}

} // namespace {}


void registerSources(Sources& s)
{
    std::unique_lock l(s.mutex);

    s.map.insert({ ServerVersion, { serverVersion } });
    s.map.insert({ OsType, { osType } });
    s.map.insert({ OsVersion, { osVersion } });
}

} // namespace Er::SystemInfo::Private {}
