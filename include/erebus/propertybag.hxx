#pragma once

#include <erebus/property.hxx>

#include <unordered_map>
#include <vector>


namespace Er
{

using PropertyMap = std::unordered_map<PropId, Property>;
using PropertyVector = std::vector<Property>;


template <IsPropertyValue PropT>
inline bool propertyPresent(const PropertyMap& bag) noexcept
{
    using Id = typename PropT::Id;

    auto it = bag.find(Id::value);
    return (it != bag.end());
}

template <IsPropertyValue PropT>
inline bool propertyPresent(const PropertyVector& bag) noexcept
{
    using Id = typename PropT::Id;

    auto it = std::find_if(bag.begin(), bag.end(), [](const Property& prop) { return prop.id == Id::value; });
    return (it != bag.end());
}


inline const Property* findProperty(const PropertyMap& bag, PropId id) noexcept
{
    auto it = bag.find(id);
    if (it != bag.end())
        return &(it->second);

    return nullptr;
}

inline Property* findProperty(PropertyMap& bag, PropId id) noexcept
{
    auto it = bag.find(id);
    if (it != bag.end())
        return &(it->second);

    return nullptr;
}

inline const Property* findProperty(const PropertyVector& bag, PropId id) noexcept
{
    auto it = std::find_if(bag.begin(), bag.end(), [id](const Property& prop) { return prop.id == id; });
    if (it != bag.end())
        return &(*it);

    return nullptr;
}

inline Property* findProperty(PropertyVector& bag, PropId id) noexcept
{
    auto it = std::find_if(bag.begin(), bag.end(), [id](Property& prop) { return prop.id == id; });
    if (it != bag.end())
        return &(*it);

    return nullptr;
}


template <IsPropertyValue PropT>
typename PropT::ValueType const* getProperty(const PropertyMap& bag) noexcept
{
    using Id = typename PropT::Id;
    using Type = typename PropT::ValueType;
    
    auto it = bag.find(Id::value);
    if (it == bag.end()) [[unlikely]]
        return nullptr;

    return std::get_if<Type>(&it->second.value);
} 

template <IsPropertyValue PropT>
typename PropT::ValueType const* getProperty(const PropertyVector& bag) noexcept
{
    using Id = typename PropT::Id;
    using Type = typename PropT::ValueType;
    
    auto it = std::find_if(bag.begin(), bag.end(), [](const Property& prop) { return prop.id == Id::value; });
    if (it == bag.end()) [[unlikely]]
        return nullptr;

    return std::get_if<Type>(&it->value);
} 


template <IsPropertyValue PropT>
typename PropT::ValueType getPropertyOr(const PropertyMap& bag, typename PropT::ValueType const& defaultValue) noexcept
{
    using Id = typename PropT::Id;
    using Type = typename PropT::ValueType;

    auto it = bag.find(Id::value);
    if (it == bag.end()) [[unlikely]]
        return defaultValue;

    auto ptr = std::get_if<Type>(&it->second.value);
    if (!ptr) [[unlikely]]
        return defaultValue;

    return *ptr;
}

template <IsPropertyValue PropT>
typename PropT::ValueType getPropertyOr(const PropertyVector& bag, typename PropT::ValueType const& defaultValue) noexcept
{
    using Id = typename PropT::Id;
    using Type = typename PropT::ValueType;

    auto it = std::find_if(bag.begin(), bag.end(), [](const Property& prop) { return prop.id == Id::value; });
    if (it == bag.end()) [[unlikely]]
        return defaultValue;

    auto ptr = std::get_if<Type>(&it->value);
    if (!ptr) [[unlikely]]
        return defaultValue;

    return *ptr;
}

template <IsPropertyValue PropT>
typename PropT::ValueType getPropertyOr(const PropertyMap& bag, typename PropT::ValueType&& defaultValue) noexcept
{
    using Id = typename PropT::Id;
    using Type = typename PropT::ValueType;

    auto it = bag.find(Id::value);
    if (it == bag.end()) [[unlikely]]
        return std::move(defaultValue);

    auto ptr = std::get_if<Type>(&it->second.value);
    if (!ptr) [[unlikely]]
        return std::move(defaultValue);

    return *ptr;
}

template <IsPropertyValue PropT>
typename PropT::ValueType getPropertyOr(const PropertyVector& bag, typename PropT::ValueType&& defaultValue) noexcept
{
    using Id = typename PropT::Id;
    using Type = typename PropT::ValueType;

    auto it = std::find_if(bag.begin(), bag.end(), [](const Property& prop) { return prop.id == Id::value; });
    if (it == bag.end()) [[unlikely]]
        return std::move(defaultValue);

    auto ptr = std::get_if<Type>(&it->value);
    if (!ptr) [[unlikely]]
        return std::move(defaultValue);

    return *ptr;
}


template <IsPropertyValue PropT>
void addProperty(PropertyMap& bag, typename PropT::ValueType const& v)
{
    bag.insert({ PropT::id, Er::Property(PropT::id, v) });
}

template <IsPropertyValue PropT>
void addProperty(PropertyVector& bag, typename PropT::ValueType const& v)
{
    bag.push_back(Er::Property(PropT::id, v));
}

template <IsPropertyValue PropT>
void addProperty(PropertyMap& bag, typename PropT::ValueType&& v)
{
    bag.insert({ PropT::id, Er::Property(PropT::id, std::move(v)) });
}

template <IsPropertyValue PropT>
void addProperty(PropertyVector& bag, typename PropT::ValueType&& v)
{
    bag.push_back(Er::Property(PropT::id, std::move(v)));
}

inline void insertProperty(PropertyMap& bag, const Property& prop)
{
    bag.insert({ prop.id, prop });
}

inline void insertProperty(PropertyMap& bag, Property&& prop)
{
    bag.insert({ prop.id, std::move(prop) });
}

inline void insertProperty(PropertyVector& bag, const Property& prop)
{
    bag.push_back(prop);
}

inline void insertProperty(PropertyVector& bag, Property&& prop)
{
    bag.push_back(std::move(prop));
}


template <typename VisitorT>
    requires std::is_invocable_v<VisitorT, const Property&>
void enumerateProperties(const PropertyMap& bag, VisitorT visitor)
{
    for (auto& pair: bag)
    {
        if constexpr (std::is_invocable_r_v<bool, VisitorT, const Property&>)
        {
            if (!visitor(pair.second))
                break;
        }
        else if constexpr(std::is_invocable_v<VisitorT, const Property&>)
        {
            visitor(pair.second);
        }
        else
        {
            ErAssert(!"unsupported visitor");
        }
    }
}

template <typename VisitorT>
    requires std::is_invocable_v<VisitorT, Property&>
void enumerateProperties(PropertyMap& bag, VisitorT visitor)
{
    for (auto& pair: bag)
    {
        if constexpr (std::is_invocable_r_v<bool, VisitorT, Property&>)
        {
            if (!visitor(pair.second))
                break;
        }
        else if constexpr(std::is_invocable_v<VisitorT, Property&>)
        {
            visitor(pair.second);
        }
        else
        {
            ErAssert(!"unsupported visitor");
        }
    }
}


template <typename VisitorT>
    requires std::is_invocable_v<VisitorT, const Property&>
void enumerateProperties(const PropertyVector& bag, VisitorT visitor)
{
    for (auto& prop: bag)
    {
        if constexpr (std::is_invocable_r_v<bool, VisitorT, const Property&>)
        {
            if (!visitor(prop))
                break;
        }
        else if constexpr(std::is_invocable_v<VisitorT, const Property&>)
        {
            visitor(prop);
        }
        else
        {
            ErAssert(!"unsupported visitor");
        }
    }
}

template <typename VisitorT>
    requires std::is_invocable_v<VisitorT, Property&>
void enumerateProperties(PropertyVector& bag, VisitorT visitor)
{
    for (auto& prop: bag)
    {
        if constexpr (std::is_invocable_r_v<bool, VisitorT, Property&>)
        {
            if (!visitor(prop))
                break;
        }
        else if constexpr(std::is_invocable_v<VisitorT, Property&>)
        {
            visitor(prop);
        }
        else
        {
            ErAssert(!"unsupported visitor");
        }
    }
}


using PropertyBag = PropertyVector;


} // namespace Er {}
