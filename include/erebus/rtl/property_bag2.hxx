#pragma once

#include <erebus/rtl/property2.hxx>

#include <vector>


namespace Er
{

using PropertyBag2 = std::vector<Property2>;


inline bool operator==(const PropertyBag2& a, const PropertyBag2& b) noexcept
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

inline void addProperty(PropertyBag2& bag, const Property2& prop)
{
    bag.push_back(prop);
}

inline void addProperty(PropertyBag2& bag, Property2&& prop)
{
    auto name = prop.name();
    bag.push_back(std::move(prop));
}

inline bool visit(const PropertyBag2& bag, auto&& visitor)
{
    for (auto& prop : bag)
    {
        if (!visit(prop, std::forward<decltype(visitor)>(visitor)))
            return false;
    }

    return true;
}

} // namespace Er {}