#include "erebus-version.h"

#include "system_info_common.hxx"

#include <erebus/rtl/format.hxx>

#include <sys/utsname.h>


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

} // namespace {}


void registerSources(Sources& s)
{
    std::unique_lock l(s.mutex);

    s.map.insert({ ServerVersion, { serverVersion } });
    s.map.insert({ OsType, { osType } });
    s.map.insert({ OsVersion, { osVersion } });
}

} // namespace Er::SystemInfo::Private {}
