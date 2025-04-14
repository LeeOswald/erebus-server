#include "system_info_common.hxx"

namespace Er
{

namespace SystemInfo
{

namespace Private
{


std::map<std::string_view, SystemInfoSource> const& sources()
{
    static std::map<std::string_view, SystemInfoSource> m = registerSources();
    return m;
}

} // namespace Private {}


ER_SERVER_EXPORT Property get(std::string_view name)
{
    auto& m = Private::sources();
    auto it = m.find(name);

    if (it == m.end())
        return Property{};

    return (it->second)(name);
}


} // namespace SystemInfo {}

} // namespace Er {}