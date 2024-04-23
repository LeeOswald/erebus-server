#pragma once

#include <erebus/stringliteral.hxx>
#include <erebus/system/time.hxx>
#include <erebus/util/crc32.hxx>

#include <iomanip>
#include <ostream>
#include <typeinfo>
#include <unordered_map>
#include <variant>


namespace Er
{

//
// Properties that can be marshaled through RPC
//

using PropId = uint32_t;
constexpr PropId InvalidPropId = PropId(-1);


template <typename T, typename = void>
struct PropertyComparator;

template <typename T, typename = void>
struct PropertyFormatter;

template <typename T>
concept SupportedPropertyType = 
    std::is_same_v<std::remove_cvref_t<T>, bool> ||
    std::is_same_v<std::remove_cvref_t<T>, int32_t> ||
    std::is_same_v<std::remove_cvref_t<T>, uint32_t> ||
    std::is_same_v<std::remove_cvref_t<T>, int64_t> ||
    std::is_same_v<std::remove_cvref_t<T>, uint64_t> ||
    std::is_same_v<std::remove_cvref_t<T>, double> ||
    std::is_same_v<std::remove_cvref_t<T>, std::string> ||
    std::is_same_v<std::remove_cvref_t<T>, Bytes>;


using PropertyValueStorage = std::variant<
    bool,
    int32_t,
    uint32_t,
    int64_t,
    uint64_t,
    double,
    std::string,
    Bytes
>;


enum class PropertyType : std::size_t
{
    Invalid = std::variant_npos,
    Bool = 0,
    Int32,
    UInt32,
    Int64,
    UInt64,
    Double,
    String,
    Bytes
};


template <typename T>
struct PropertyTypeFrom;

template <>
struct PropertyTypeFrom<bool>
{
    static constexpr PropertyType type = PropertyType::Bool;
};

template <>
struct PropertyTypeFrom<int32_t>
{
    static constexpr PropertyType type = PropertyType::Int32;
};

template <>
struct PropertyTypeFrom<uint32_t>
{
    static constexpr PropertyType type = PropertyType::UInt32;
};

template <>
struct PropertyTypeFrom<int64_t>
{
    static constexpr PropertyType type = PropertyType::Int64;
};

template <>
struct PropertyTypeFrom<uint64_t>
{
    static constexpr PropertyType type = PropertyType::UInt64;
};

template <>
struct PropertyTypeFrom<double>
{
    static constexpr PropertyType type = PropertyType::Double;
};

template <>
struct PropertyTypeFrom<std::string>
{
    static constexpr PropertyType type = PropertyType::String;
};

template <>
struct PropertyTypeFrom<Bytes>
{
    static constexpr PropertyType type = PropertyType::Bytes;
};



template <SupportedPropertyType ValueT, PropId PrId, StringLiteral PrIdStr, StringLiteral PrName, class ComparatorT = PropertyComparator<ValueT>, class FormatterT = PropertyFormatter<ValueT>>
class PropertyInfo
{
public:
    using ValueType = std::decay_t<ValueT>;
    using Id = std::integral_constant<PropId, PrId>;
    using Comparator = ComparatorT;
    using Formatter = FormatterT;
    
    static constexpr PropertyType type() noexcept
    {
        return PropertyTypeFrom<ValueType>::type;
    }

    static constexpr const std::type_info& type_info() noexcept
    {
        return typeid(ValueT);
    }

    static constexpr PropId id() noexcept
    {
        return Id::value;
    }

    static constexpr const char* id_str() noexcept
    {
        return fromStringLiteral<PrIdStr>();
    }
    
    static constexpr const char* name() noexcept
    {
        return fromStringLiteral<PrName>();
    }
};


struct PropertyValueTag
{
};


template <SupportedPropertyType ValueT, PropId PrId, StringLiteral PrIdStr, StringLiteral PrName, class ComparatorT = PropertyComparator<ValueT>, class FormatterT = PropertyFormatter<ValueT>>
class PropertyValue final
    : public PropertyInfo<ValueT, PrId, PrIdStr, PrName, ComparatorT, FormatterT>
{
public:
    using Base = PropertyInfo<ValueT, PrId, PrIdStr, PrName, ComparatorT, FormatterT>;
    using ValueType = typename Base::ValueType;
    using Id = Base::Id;
    using Comparator = ComparatorT;
    using Formatter = typename Base::Formatter;
    using Tag = PropertyValueTag;

    PropertyValue() noexcept(noexcept(ValueType())) = default;

    constexpr PropertyValue(const ValueT& value) noexcept(std::is_nothrow_constructible_v<ValueType, const ValueT&>)
        : m_value(value)
    {}

    constexpr PropertyValue(ValueT&& value) noexcept(std::is_nothrow_constructible_v<ValueType, ValueT&&>)
        : m_value(std::move(value))
    {}

    constexpr ValueType&& value() && noexcept
    {
        return std::move(m_value);
    }

    constexpr ValueType& value() & noexcept
    {
        return m_value;
    }

private:
    ValueType m_value;
};


template <typename T>
concept IsPropertyValue =
    requires
{
    typename T::ValueType;
    typename T::Comparator;
    typename T::Formatter;

    requires std::same_as<typename T::Tag, PropertyValueTag>;
    requires std::same_as<std::decay_t<typename T::Id::value_type>, PropId>;
    { T::type_info() } -> std::same_as<const std::type_info&>;
    { T::id() } -> std::same_as<PropId>;
    { T::id_str() } -> std::same_as<const char*>;
    { T::name() } -> std::same_as<const char*>;
};



struct IPropertyInfo;


struct EREBUS_EXPORT Property
{
    Property() noexcept = default;

    template <IsPropertyValue PropertyValueT>
    Property(PropertyValueT&& pv) noexcept(noexcept(std::is_nothrow_constructible_v<PropertyValueStorage, decltype(std::forward<PropertyValueT>(pv).value())>))
        : id(pv.id())
        , value(std::forward<PropertyValueT>(pv).value())
        , type(PropertyTypeFrom<typename PropertyValueT::ValueType>::type)
    {
#if ER_DEBUG
        checkProperty();
#endif
    }

    template <SupportedPropertyType ValueT>
    Property(PropId id, ValueT&& value, IPropertyInfo* info = nullptr) noexcept(noexcept(std::is_nothrow_constructible_v<PropertyValueStorage, decltype(std::forward<ValueT>(value))>))
        : id(id)
        , value(std::forward<ValueT>(value))
        , type(static_cast<PropertyType>(this->value.index()))
        , info(info)
    {
#if ER_DEBUG
        checkProperty();
#endif
    }

    PropId id = InvalidPropId;
    PropertyValueStorage value;
    PropertyType type = PropertyType::Invalid;
    mutable IPropertyInfo* info = nullptr;

private:
#if ER_DEBUG
    void checkProperty();
#endif
};


struct AlwaysEqualPropertyComparator
{
    bool operator()(const Property& a, const Property& b) { return true; }
};

struct NeverEqualPropertyComparator
{
    bool operator()(const Property& a, const Property& b) { return false; }
};

struct BytesComparator
{
    bool operator()(const Property& a, const Property& b) { return std::get<Bytes>(a.value) == std::get<Bytes>(b.value); }
};

template <std::equality_comparable T>
struct PropertyComparator<T>
{
    bool operator()(const Property& a, const Property& b) { return std::get<T>(a.value) == std::get<T>(b.value); }
};


struct NullPropertyFormatter
{
    void operator()([[maybe_unused]] const Property& v, [[maybe_unused]] std::ostream& s) {  }
};

template <typename T>
struct PropertyFormatter<T, std::enable_if_t<std::is_same<T, bool>::value>>
{
    void operator()(const Property& v, std::ostream& s) { s << std::boolalpha << std::get<T>(v.value); }
};

template <typename T>
struct PropertyFormatter<T, std::enable_if_t<std::is_integral<T>::value && !std::is_same<T, bool>::value>>
{
    void operator()(const Property& v, std::ostream& s) { s << std::get<T>(v.value); }
};

template <typename T>
struct PropertyFormatter<T, std::enable_if_t<std::is_same<T, std::string>::value>>
{
    void operator()(const Property& v, std::ostream& s) { s << std::get<T>(v.value); }
};


enum class TimeZone
{
    Utc,
    Local
};

template <StringLiteral Format, TimeZone Zone>
struct TimeFormatter
{
    static constexpr const char* format = fromStringLiteral<Format>();

    void operator()(const Property& v, std::ostream& s) 
    {
        auto packed = std::get<uint64_t>(v.value);
        Er::System::Time unpacked;
        if constexpr (Zone == TimeZone::Utc)
            unpacked = Er::System::Time::gmt(packed);
        else
            unpacked = Er::System::Time::local(packed);

        struct tm t = {};
        t.tm_year = unpacked.year;
        t.tm_mon = unpacked.month;
        t.tm_mday = unpacked.day;
        t.tm_hour = unpacked.hour;
        t.tm_min = unpacked.minute;
        t.tm_sec = unpacked.second;

        s << std::put_time(&t, format);
    }
};

struct IPropertyInfo
{
    using Ptr = std::shared_ptr<IPropertyInfo>;

    virtual PropertyType type() const noexcept = 0;
    virtual const std::type_info& type_info() const noexcept = 0;
    virtual PropId id() const noexcept = 0;
    virtual const char* id_str() const noexcept = 0;
    virtual const char* name() const noexcept = 0;
    virtual void format(const Property& v, std::ostream& s) const = 0;
    virtual bool equal(const Property& a, const Property& b) const noexcept = 0;

protected:
    virtual ~IPropertyInfo() {}
};


template <class PropertyInfoT>
struct PropertyInfoWrapper
    : public IPropertyInfo
{
    using PropertyInfo = PropertyInfoT;
    using Comparator = typename PropertyInfo::Comparator;
    using Formatter = typename PropertyInfo::Formatter;

    PropertyType type() const noexcept override
    {
        return PropertyInfo::type();
    }

    const std::type_info& type_info() const noexcept override
    {
        return PropertyInfo::type_info();
    }

    PropId id() const noexcept override
    {
        return PropertyInfo::id();
    }

    const char* id_str() const noexcept override
    {
        return PropertyInfo::id_str();
    }

    const char* name() const noexcept override
    {
        return PropertyInfo::name();
    }

    void format(const Property& v, std::ostream& s) const override
    {
        Formatter f;
        f(v, s);
    }

    bool equal(const Property& a, const Property& b) const noexcept override
    {
        Comparator c;
        return c(a, b);
    }
};


using PropertyBag = std::unordered_map<PropId, Property>;


inline bool propertyPresent(const PropertyBag& bag, PropId id) noexcept
{
    auto it = bag.find(id);
    return (it != bag.end());
}


template <IsPropertyValue PropT>
std::optional<typename PropT::ValueType> getProperty(const PropertyBag& bag)
{
    using Id = typename PropT::Id;
    auto it = bag.find(Id::value);
    if (it == bag.end())
        return std::nullopt;

    return std::optional<typename PropT::ValueType>(std::get<typename PropT::ValueType>(it->second.value));
} 


template <IsPropertyValue PropT>
typename PropT::ValueType getProperty(const PropertyBag& bag, typename PropT::ValueType&& defaultValue)
{
    using Id = typename PropT::Id;
    auto it = bag.find(Id::value);
    if (it == bag.end())
        return typename PropT::ValueType(std::forward<typename PropT::ValueType>(defaultValue));

    return std::get<typename PropT::ValueType>(it->second.value);
}


template <IsPropertyValue PropT>
void addProperty(PropertyBag& bag, typename PropT::ValueType const& v)
{
    bag.insert({ PropT::id(), Er::Property(PropT::id(), v) });
}

template <IsPropertyValue PropT>
void addProperty(PropertyBag& bag, typename PropT::ValueType&& v)
{
    bag.insert({ PropT::id(), Er::Property(PropT::id(), std::move(v)) });
}


} // namespace Er {}


#define ER_PROPID_(s)   Er::Util::crc32(s)
#define ER_PROPID(s)   Er::Util::crc32(s), s

