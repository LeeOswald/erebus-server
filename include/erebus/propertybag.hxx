#pragma once

#include <erebus/property.hxx>

#include <vector>


namespace Er
{

using PropertyBag = std::vector<Property>;


inline auto propertyCount(const PropertyBag& bag) noexcept
{
    return bag.size();
}

inline void clearPropertyBag(PropertyBag& bag) noexcept
{
    bag.clear();
}

inline void reservePropertyBag(PropertyBag& bag, std::size_t size)
{
    bag.reserve(size);
}

inline void resizePropertyBag(PropertyBag& bag, std::size_t size)
{
    bag.resize(size);
}

template <IsPropertyValue PropT>
void addProperty(PropertyBag& bag, typename PropT::ValueType const& v)
{
    bag.push_back(Er::Property(PropT::id(), v));
}

template <IsPropertyValue PropT>
void addProperty(PropertyBag& bag, typename PropT::ValueType&& v)
{
    bag.push_back(Er::Property(PropT::id(), std::move(v)));
}

inline void addProperty(PropertyBag& bag, const Property& prop)
{
    bag.push_back(prop);
}

inline void addProperty(PropertyBag& bag, Property&& prop)
{
    bag.push_back(std::move(prop));
}

template <IsPropertyValue PropT>
void setPropertyValueAt(PropertyBag& bag, std::size_t index, typename PropT::ValueType const& v)
{
    using Id = typename PropT::Id;
    using Type = typename PropT::ValueType;

    if (bag.size() <= index)
    {
        bag.resize(index + 1);
    }

    bag[index] = Property(Id::value, v);
} 

template <IsPropertyValue PropT>
void setPropertyValueAt(PropertyBag& bag, std::size_t index, typename PropT::ValueType&& v)
{
    using Id = typename PropT::Id;
    using Type = typename PropT::ValueType;

    if (bag.size() <= index)
    {
        bag.resize(index + 1);
    }

    bag[index] = Property(Id::value, std::move(v));
}

template <IsPropertyValue PropT>
bool updatePropertyValueAt(PropertyBag& bag, std::size_t index, typename PropT::ValueType const& v)
{
    using Id = typename PropT::Id;
    using Type = typename PropT::ValueType;
    
    ErAssert(index < bag.size());
    ErAssert(bag[index].id == Id::value);

    if (std::get<Type>(bag[index].value) != v)
    {        
        bag[index] = Property(Id::value, v);
        return true;
    }

    return false;
} 

template <IsPropertyValue PropT>
bool updatePropertyValueAt(PropertyBag& bag, std::size_t index, typename PropT::ValueType&& v)
{
    using Id = typename PropT::Id;
    using Type = typename PropT::ValueType;
    
    ErAssert(index < bag.size());
    ErAssert(bag[index].id == Id::value);

    if (std::get<Type>(bag[index].value) != v)
    {        
        bag[index] = Property(Id::value, std::move(v));
        return true;
    }

    return false;
} 

template <IsPropertyValue PropT>
inline bool propertyPresent(const PropertyBag& bag) noexcept
{
    using Id = typename PropT::Id;

    auto it = std::find_if(bag.begin(), bag.end(), [](const Property& prop) { return prop.id == Id::value; });
    return (it != bag.end());
}


inline const Property* getProperty(const PropertyBag& bag, PropId id) noexcept
{
    auto it = std::find_if(bag.begin(), bag.end(), [id](const Property& prop) { return prop.id == id; });
    if (it != bag.end())
        return &(*it);

    return nullptr;
}

inline Property* getProperty(PropertyBag& bag, PropId id) noexcept
{
    auto it = std::find_if(bag.begin(), bag.end(), [id](Property& prop) { return prop.id == id; });
    if (it != bag.end())
        return &(*it);

    return nullptr;
}

template <IsPropertyValue PropT>
typename PropT::ValueType const* getPropertyValue(const PropertyBag& bag) noexcept
{
    using Id = typename PropT::Id;
    using Type = typename PropT::ValueType;
    
    auto it = std::find_if(bag.begin(), bag.end(), [](const Property& prop) { return prop.id == Id::value; });
    if (it == bag.end()) [[unlikely]]
        return nullptr;

    return std::get_if<Type>(&it->value);
} 

template <IsPropertyValue PropT>
typename PropT::ValueType getPropertyValueOr(const PropertyBag& bag, typename PropT::ValueType const& defaultValue) noexcept
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
typename PropT::ValueType getPropertyValueOr(const PropertyBag& bag, typename PropT::ValueType&& defaultValue) noexcept
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


template <typename VisitorT>
    requires std::is_invocable_v<VisitorT, const Property&>
void enumerateProperties(const PropertyBag& bag, VisitorT visitor)
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
void enumerateProperties(PropertyBag& bag, VisitorT visitor)
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

} // namespace Er {}
