#pragma once

#include <erebus/property.hxx>


namespace Er
{

using PropertyBag = std::unordered_map<PropId, Property>;

template <IsPropertyValue PropT>
inline bool propertyPresent(const PropertyBag& bag) noexcept
{
    using Id = typename PropT::Id;

    auto it = bag.find(Id::value);
    return (it != bag.end());
}

template <IsPropertyValue PropT>
typename PropT::ValueType const* getProperty(const PropertyBag& bag)
{
    using Id = typename PropT::Id;
    using Type = typename PropT::ValueType;
    
    auto it = bag.find(Id::value);
    if (it == bag.end()) [[unlikely]]
        return nullptr;

    return std::get_if<Type>(&it->second.value);
} 

template <IsPropertyValue PropT>
typename PropT::ValueType getPropertyOr(const PropertyBag& bag, typename PropT::ValueType const& defaultValue)
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
typename PropT::ValueType getPropertyOr(const PropertyBag& bag, typename PropT::ValueType&& defaultValue)
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
bool addProperty(PropertyBag& bag, typename PropT::ValueType const& v)
{
    auto r = bag.insert({ PropT::id, Er::Property(PropT::id, v) });
    return r.second;
}

template <IsPropertyValue PropT>
bool addProperty(PropertyBag& bag, typename PropT::ValueType&& v)
{
    auto r = bag.insert({ PropT::id, Er::Property(PropT::id, std::move(v)) });
    return r.second;
}


} // namespace Er {}
