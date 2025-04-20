#include "erebus-version.h"
#include "system_info_common.hxx"

#include <erebus/rtl/system/unwindows.h>

namespace Er
{

namespace SystemInfo
{

namespace Private
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

SystemInfoSources registerSources()
{
    std::map<std::string_view, SystemInfoSource> m;

    m.insert({ ServerVersion, { serverVersion } });
    m.insert({ OsType, { osType } });
    m.insert({ OsVersion, { osVersion } });
    return m;
}


} // namespace Private {}

} // namespace SystemInfo {}

} // namespace Er {}