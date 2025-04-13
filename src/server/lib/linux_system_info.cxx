#include <erebus/rtl/format.hxx>
#include <erebus/server/system_info.hxx>

#include <functional>
#include <map>

#include <sys/utsname.h>

#include "erebus-version.h"

namespace Er
{

namespace SystemInfo
{

namespace
{

using SystemInfoSource = std::function<Property(std::string_view)>;

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

auto registerSources()
{
    std::map<std::string_view, SystemInfoSource> m;

    m.insert({ ServerVersion, { serverVersion } });
    m.insert({ OsType, { osType } });
    m.insert({ OsVersion, { osVersion } });
    return m;
}

std::map<std::string_view, SystemInfoSource> const& sources()
{
    static std::map<std::string_view, SystemInfoSource> m = registerSources();
    return m;
}

} // namespace {}


ER_SERVER_EXPORT Property get(std::string_view name)
{
    auto& m = sources();
    auto it = m.find(name);

    if (it == m.end())
        return Property{};

    return (it->second)(name);
}


} // namespace SystemInfo {}

} // namespace Er {}