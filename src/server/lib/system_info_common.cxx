#include "system_info_common.hxx"

#include <erebus/rtl/util/pattern.hxx>

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


ER_SERVER_EXPORT PropertyBag get(std::string_view name)
{
    PropertyBag bag;

    auto& m = Private::sources();
    
    
    if (std::find_if(name.begin(), name.end(), [](char c) { return (c == '?') || (c == '*'); }) == name.end())
    {
        // exact name?
        auto it = m.find(name);
        if (it != m.end())
        {
            bag.push_back((it->second)(name));
        }
    }
    else
    {
        // wildcard?
        for (auto item : m)
        {
            if (Er::Util::matchString(std::string_view{ item.first }, name))
            {
                bag.push_back((item.second)(item.first));
            }
        }
    }
    
    return bag;
}


} // namespace SystemInfo {}

} // namespace Er {}