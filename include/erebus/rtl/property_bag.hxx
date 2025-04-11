#pragma once

#include <erebus/rtl/property.hxx>

#include <vector>


namespace Er
{

using PropertyBag = std::vector<Property>;


inline bool operator==(const PropertyBag& a, const PropertyBag& b) noexcept
{
    if (a.size() != b.size())
        return false;

    auto ita = a.begin();
    auto itb = b.begin();
    while (ita != a.end())
    {
        if (*ita != *itb)
            return false;

        ++ita;
        ++itb;
    }

    return true;
}

inline void addProperty(PropertyBag& bag, const Property& prop)
{
    bag.push_back(prop);
}

inline void addProperty(PropertyBag& bag, Property&& prop)
{
    auto name = prop.name();
    bag.push_back(std::move(prop));
}

inline bool visit(const PropertyBag& bag, auto&& visitor)
{
    for (auto& prop : bag)
    {
        if (!visit(prop, std::forward<decltype(visitor)>(visitor)))
            return false;
    }

    return true;
}

inline const Property* findProperty(const PropertyBag& bag, std::string_view name) noexcept
{
    for (auto& prop : bag)
    {
        if (prop.name() == name)
            return &prop;
    }

    return nullptr;
}

} // namespace Er {}