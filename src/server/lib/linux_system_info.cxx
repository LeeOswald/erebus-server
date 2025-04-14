#include "erebus-version.h"
#include "system_info_common.hxx"

#include <sys/utsname.h>


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
    struct utsname u = {};
    if (::uname(&u) == 0)
    {
        return Property{ OsType, std::string{u.sysname} };
    }

    return {};
}

Property osVersion(std::string_view)
{
    struct utsname u = {};
    if (::uname(&u) == 0)
    {
        return Property{ OsType, std::string{u.release} };
    }

    return {};
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