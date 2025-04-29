#pragma once

#include <erebus/rtl/property.hxx>


namespace Er
{

using PropertyBag = Property::Vector;

using PropertyVector = Property::Vector;
using PropertyMap = Property::Map;


inline bool visit(const Property& prop, auto&& visitor)
{
    auto ty = prop.type();
    switch (ty)
    {
    case Er::Property::Type::Empty:
        return visitor(prop, Empty{});
    case Er::Property::Type::Bool:
        ErAssert(prop.getBool());
        return visitor(prop, *prop.getBool());
    case Er::Property::Type::Int32:
        ErAssert(prop.getInt32());
        return visitor(prop, *prop.getInt32());
    case Er::Property::Type::UInt32:
        ErAssert(prop.getUInt32());
        return visitor(prop, *prop.getUInt32());
    case Er::Property::Type::Int64:
        ErAssert(prop.getInt64());
        return visitor(prop, *prop.getInt64());
    case Er::Property::Type::UInt64:
        ErAssert(prop.getUInt64());
        return visitor(prop, *prop.getUInt64());
    case Er::Property::Type::Double:
        ErAssert(prop.getDouble());
        return visitor(prop, *prop.getDouble());
    case Er::Property::Type::String:
        ErAssert(prop.getString());
        return visitor(prop, *prop.getString());
    case Er::Property::Type::Binary:
        ErAssert(prop.getBinary());
        return visitor(prop, *prop.getBinary());
    case Er::Property::Type::Map:
        ErAssert(prop.getMap());
        return visitor(prop, *prop.getMap());
    case Er::Property::Type::Vector:
        ErAssert(prop.getVector());
        return visitor(prop, *prop.getVector());
    }

    ErAssert(!"Unkwnown property type");
    return false;
}

[[nodiscard]] inline bool operator==(const PropertyMap& a, const PropertyMap& b) noexcept
{
    if (a.size() != b.size())
        return false;

    auto ita = a.begin();
    auto itb = b.begin();
    while (ita != a.end())
    {
        if (ita->first != itb->first)
            return false;

        if (ita->second != itb->second)
            return false;

        ++ita;
        ++itb;
    }

    return true;
}

[[nodiscard]] inline bool operator==(const PropertyVector& a, const PropertyVector& b) noexcept
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


inline void addProperty(PropertyMap& map, const Property& prop)
{
    map.insert({ prop.name(), prop });
}

inline void addProperty(PropertyMap& map, Property&& prop)
{
    auto name = prop.name();
    map.insert({ std::move(name), std::move(prop) });
}

inline void addProperty(PropertyVector& bag, const Property& prop)
{
    bag.push_back(prop);
}

inline void addProperty(PropertyVector& bag, Property&& prop)
{
    auto name = prop.name();
    bag.push_back(std::move(prop));
}


inline bool visit(const PropertyMap& m, auto&& visitor)
{
    for (auto& entry : m)
    {
        if (!visit(entry.second, std::forward<decltype(visitor)>(visitor)))
            return false;
    }

    return true;
}

inline bool visit(const PropertyVector& bag, auto&& visitor)
{
    for (auto& prop : bag)
    {
        if (!visit(prop, std::forward<decltype(visitor)>(visitor)))
            return false;
    }

    return true;
}

[[nodiscard]] inline const Property* findProperty(const PropertyMap& bag, std::string_view name, std::optional<Property::Type> type = std::nullopt) noexcept
{
    const Property* prop = nullptr;

    auto it = bag.find(name);
    if (it != bag.end())
    {
        ErAssert(it->second.name() == name);
        prop = &it->second;

        if (type && prop->type() != *type)
            return nullptr;
    }
    
    return prop;
}

[[nodiscard]] inline const Property* findProperty(const PropertyVector& bag, std::string_view name, std::optional<Property::Type> type = std::nullopt) noexcept
{
    const Property* prop = nullptr;

    for (auto& pr : bag)
    {
        if (pr.name() == name)
        {
            prop = &pr;

            if (type && prop->type() != *type)
                return nullptr;

            break;
        }
    }

    return prop;
}


[[nodiscard]] ER_RTL_EXPORT Property loadJson(std::string_view json, std::size_t maxDepth = 256);

} // namespace Er {}