#pragma once

#include <erebus/empty.hxx>
#include <erebus/stringliteral.hxx>
#include <erebus/system/time.hxx>
#include <erebus/util/crc32.hxx>

#include <iomanip>
#include <ostream>
#include <variant>


namespace Er
{

//
// Properties that can be marshaled through RPC
//

using PropId = uint32_t;
constexpr PropId InvalidPropId = PropId(-1);

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
    Empty,
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
    Empty = 0,
    Bool,
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



template <SupportedPropertyType ValueT, PropId PrId, StringLiteral PrIdStr, StringLiteral PrName, class FormatterT = PropertyFormatter<ValueT>>
struct PropertyInfo
{
    using ValueType = std::decay_t<ValueT>;
    using Id = std::integral_constant<PropId, PrId>;
    using Formatter = FormatterT;
    
    static constexpr PropertyType type = PropertyTypeFrom<ValueType>::type;
    static constexpr PropId id = Id::value;
    static constexpr const char* id_str = fromStringLiteral<PrIdStr>();
    static constexpr const char* name = fromStringLiteral<PrName>();

    static void format(const PropertyValueStorage& v, std::ostream& s)
    {
        auto p = std::get_if<ValueType>(&v);
        ErAssert(p);
        if (p)
        {
            Formatter f;
            f(*p, s);
        }
    }
};


template <SupportedPropertyType ValueT, PropId PrId, StringLiteral PrIdStr, StringLiteral PrName, class FormatterT = PropertyFormatter<ValueT>>
struct PropertyValue final
    : public PropertyInfo<ValueT, PrId, PrIdStr, PrName, FormatterT>
{
    PropertyValue() noexcept(noexcept(std::is_nothrow_constructible_v<ValueT>))
        : value()
    {}

    constexpr explicit PropertyValue(const ValueT& value) noexcept(std::is_nothrow_copy_constructible_v<ValueT>)
        : value(value)
    {}

    constexpr explicit PropertyValue(ValueT&& value) noexcept(std::is_nothrow_move_constructible_v<ValueT>)
        : value(std::move(value))
    {}

    ValueT value;
};


template <typename T>
concept IsPropertyValue =
    requires
{
    typename T::ValueType;

    requires std::same_as<std::remove_cvref_t<decltype(std::declval<T>().id)>, PropId>;
    requires std::same_as<std::remove_cvref_t<decltype(std::declval<T>().value)>, typename T::ValueType>;
};



struct IPropertyInfo;

EREBUS_EXPORT std::shared_ptr<IPropertyInfo> lookupProperty(PropId id) noexcept;
EREBUS_EXPORT std::shared_ptr<IPropertyInfo> lookupProperty(const char* id) noexcept;



struct NullPropertyFormatter
{
    template <typename T>
    void operator()([[maybe_unused]] const T& v, [[maybe_unused]] std::ostream& s) {  }
};

template <typename T>
struct PropertyFormatter<T, std::enable_if_t<std::is_same<T, bool>::value>>
{
    void operator()(const T& v, std::ostream& s) { s << std::boolalpha << v; }
};

template <typename T>
struct PropertyFormatter<T, std::enable_if_t<std::is_integral<T>::value && !std::is_same<T, bool>::value>>
{
    void operator()(const T& v, std::ostream& s) { s << v; }
};

template <typename T>
struct PropertyFormatter<T, std::enable_if_t<std::is_floating_point<T>::value>>
{
    explicit PropertyFormatter(int precision = 3)
        : precision(precision)
    {}

    void operator()(const T& v, std::ostream& s) { s << std::fixed << std::setprecision(precision) << v; }

    int precision;
};

template <typename T>
struct PropertyFormatter<T, std::enable_if_t<std::is_same<T, std::string>::value>>
{
    void operator()(const T& v, std::ostream& s) { s << v; }
};

template <typename T>
struct PropertyFormatter<T, std::enable_if_t<std::is_same<T, Bytes>::value>>
{
    void operator()(const T& v, std::ostream& s) { s << v; }
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

    void operator()(uint64_t v, std::ostream& s) 
    {
        Er::System::Time unpacked;
        if constexpr (Zone == TimeZone::Utc)
            unpacked = Er::System::Time::gmt(v);
        else
            unpacked = Er::System::Time::local(v);

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


struct EREBUS_EXPORT Property
{
    Property() noexcept = default;

    template <IsPropertyValue PropertyValueT>
    explicit Property(const PropertyValueT& pv) noexcept(noexcept(std::is_nothrow_constructible_v<PropertyValueStorage, const PropertyValueT&>))
        : value(pv.value)
        , id(pv.id)
    {
    }

    template <IsPropertyValue PropertyValueT>
    explicit Property(PropertyValueT&& pv) noexcept(noexcept(std::is_nothrow_constructible_v<PropertyValueStorage, PropertyValueT&&>))
        : value(std::move(pv.value))
        , id(pv.id)
    {
    }

    template <SupportedPropertyType ValueT>
    explicit Property(PropId id, const ValueT& value) noexcept(noexcept(std::is_nothrow_constructible_v<PropertyValueStorage, const ValueT&>))
        : value(value)
        , id(id)
    {
    }

    template <SupportedPropertyType ValueT>
    explicit Property(PropId id, ValueT&& value) noexcept(noexcept(std::is_nothrow_constructible_v<PropertyValueStorage, ValueT&&>))
        : value(std::move(value))
        , id(id)
    {
    }

    friend void swap(Property& a, Property& b) noexcept(noexcept(std::is_nothrow_swappable_v<PropertyValueStorage>))
    {
        using std::swap;
        swap(a.id, b.id);
        a.value.swap(b.value);
    }

    Property(const Property& o)
        : value(o.value)
        , id(o.id)
    {
    }

    Property& operator=(const Property& o)
    {
        Property tmp(o);
        swap(*this, tmp);
        return *this;
    }

    Property(Property&& o) noexcept(noexcept(std::is_nothrow_move_constructible_v<PropertyValueStorage>))
        : value(std::move(o.value))
        , id(o.id)
    {
    }

    Property& operator=(Property&& o) noexcept(noexcept(std::is_nothrow_move_constructible_v<PropertyValueStorage>))
    {
        Property tmp(std::move(o));
        swap(*this, tmp);
        return *this;
    }

    constexpr PropertyType type() const noexcept
    {
        return static_cast<PropertyType>(value.index());
    }

    constexpr bool empty() const noexcept
    {
        return (value.index() == 0) || (value.index() == std::variant_npos);
    }

    friend auto operator==(const Property& a, const Property& b) noexcept
    {
        return a.value == b.value;
    }

    friend auto operator<=>(const Property& a, const Property& b) noexcept
    {
        return a.value <=> b.value;
    }

    const void* data() const noexcept
    {
        switch (type())
        {
        case PropertyType::Invalid: return nullptr;
        case PropertyType::Empty: return nullptr;
        case PropertyType::Bool: return std::get_if<bool>(&value);
        case PropertyType::Int32: return std::get_if<int32_t>(&value);
        case PropertyType::UInt32: return std::get_if<uint32_t>(&value);
        case PropertyType::Int64: return std::get_if<int64_t>(&value);
        case PropertyType::UInt64: return std::get_if<uint64_t>(&value);
        case PropertyType::Double: return std::get_if<double>(&value);
        case PropertyType::String: 
        {
            auto v = std::get_if<std::string>(&value);
            return v ? v->data() : nullptr;
        }
        case PropertyType::Bytes: 
        {
            auto v = std::get_if<Bytes>(&value);
            return v ? v->data() : nullptr;
        }
        }
        ErAssert(!"Unsupported property type");
        return nullptr;
    }

    std::size_t size() const noexcept
    {
        switch (type())
        {
        case PropertyType::Invalid: return 0;
        case PropertyType::Empty: return 0;
        case PropertyType::Bool: return sizeof(bool);
        case PropertyType::Int32: return sizeof(int32_t);
        case PropertyType::UInt32: return sizeof(uint32_t);
        case PropertyType::Int64: return sizeof(int64_t);
        case PropertyType::UInt64: return sizeof(uint64_t);
        case PropertyType::Double: return sizeof(double);
        case PropertyType::String: 
        {
            auto v = std::get_if<std::string>(&value);
            return v ? v->size() : 0;
        }
        case PropertyType::Bytes: 
        {
            auto v = std::get_if<Bytes>(&value);
            return v ? v->size() : 0;
        }
        }
        ErAssert(!"Unsupported property type");
        return 0;
    }

    void format(std::ostream& s) const
    {
        switch (type())
        {
        case PropertyType::Invalid: s << "<invalid>"; break;
        case PropertyType::Empty: s << "<empty>"; break;
        case PropertyType::Bool: PropertyFormatter<bool>()(std::get<bool>(value), s); break;
        case PropertyType::Int32: PropertyFormatter<int32_t>()(std::get<int32_t>(value), s); break;
        case PropertyType::UInt32: PropertyFormatter<uint32_t>()(std::get<uint32_t>(value), s); break;
        case PropertyType::Int64: PropertyFormatter<int64_t>()(std::get<int64_t>(value), s); break;
        case PropertyType::UInt64: PropertyFormatter<uint64_t>()(std::get<uint64_t>(value), s); break;
        case PropertyType::Double: PropertyFormatter<double>()(std::get<double>(value), s); break;
        case PropertyType::String: PropertyFormatter<std::string>()(std::get<std::string>(value), s); break;
        case PropertyType::Bytes: PropertyFormatter<Bytes>()(std::get<Bytes>(value), s); break;
        default: s << "<\?\?\?>";
        }
    }

    PropertyValueStorage value;
    PropId id = InvalidPropId;
};


struct IPropertyInfo
{
    using Ptr = std::shared_ptr<IPropertyInfo>;

    virtual PropertyType type() const noexcept = 0;
    virtual PropId id() const noexcept = 0;
    virtual const char* id_str() const noexcept = 0;
    virtual const char* name() const noexcept = 0;
    virtual void format(const Property& v, std::ostream& s) const = 0;

protected:
    virtual ~IPropertyInfo() {}
};


template <class PropertyInfoT>
struct PropertyInfoWrapper
    : public IPropertyInfo
{
    using PropertyInfo = PropertyInfoT;
    using Formatter = typename PropertyInfo::Formatter;

    PropertyType type() const noexcept override
    {
        return PropertyInfo::type;
    }

    PropId id() const noexcept override
    {
        return PropertyInfo::id;
    }

    const char* id_str() const noexcept override
    {
        return PropertyInfo::id_str;
    }

    const char* name() const noexcept override
    {
        return PropertyInfo::name;
    }

    void format(const Property& v, std::ostream& s) const override
    {
        if (v.type() == PropertyType::Invalid) [[unlikely]]
        {
            s << "<\?\?\?>";
            return;
        }

        PropertyInfo::format(v.value, s);
    }
};


} // namespace Er {}


#define ER_PROPID_(s)   Er::Util::crc32(s)
#define ER_PROPID(s)   Er::Util::crc32(s), s

