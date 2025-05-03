#include "system_info_common.hxx"

#include <erebus/rtl/util/pattern.hxx>


namespace Er::Server::SystemInfo
{


ER_SERVER_EXPORT PropertyBag get(std::string_view name)
{
    PropertyBag bag;

    auto& sources = Private::Sources::instance();
    std::shared_lock l(sources.mutex);
    
    auto& m = sources.map;
    
    
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

ER_SERVER_EXPORT void registerSource(std::string_view name, Source&& src)
{
    auto& sources = Private::Sources::instance();
    std::unique_lock l(sources.mutex);
    
    auto& m = sources.map;
    m.insert({ name, std::move(src) });
}


} // namespace Er::Server::SystemInfo {}
