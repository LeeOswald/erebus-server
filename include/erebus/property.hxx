#pragma once

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
struct PropertyInfo
{
    using ValueType = std::decay_t<ValueT>;
    using Id = std::integral_constant<PropId, PrId>;
    using Comparator = ComparatorT;
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

    static bool equal(const PropertyValueStorage& a, const PropertyValueStorage& b) noexcept
    {
        auto pa = std::get_if<ValueType>(&a);
        if (!pa)
            return false;

        auto pb = std::get_if<ValueType>(&b);
        if (!pb)
            return false;

        Comparator c;
        return c(*pa, *pb);
    }

    static constexpr const void* data(const PropertyValueStorage& v) noexcept
    {
        if constexpr (std::is_same_v<ValueType, std::string>)
        {
            auto s = std::get_if<std::string>(&v);
            return s ? s->data() : nullptr;
        }
        else if constexpr (std::is_same_v<ValueType, Bytes>)
        {
            auto b = std::get_if<Bytes>(&v);
            return b ? b->data() : nullptr;
        }
        else
        {
            return std::get_if<ValueType>(&v);
        }
    }

    static constexpr std::size_t size(const PropertyValueStorage& v) noexcept
    {
        if constexpr (std::is_same_v<ValueType, std::string>)
        {
            auto s = std::get_if<std::string>(&v);
            return s ? s->size() : 0;
        }
        else if constexpr (std::is_same_v<ValueType, Bytes>)
        {
            auto b = std::get_if<Bytes>(&v);
            return b ? b->size() : 0;
        }
        else
        {
            auto p = std::get_if<ValueType>(&v);
            return p ? sizeof(ValueType) : 0;
        }
    }
};


template <SupportedPropertyType ValueT, PropId PrId, StringLiteral PrIdStr, StringLiteral PrName, class ComparatorT = PropertyComparator<ValueT>, class FormatterT = PropertyFormatter<ValueT>>
struct PropertyValue final
    : public PropertyInfo<ValueT, PrId, PrIdStr, PrName, ComparatorT, FormatterT>
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


template <std::equality_comparable T>
struct PropertyComparator<T>
{
    bool operator()(const T& a, const T& b) { return a == b; }
};


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
        : id(pv.id)
        , type(PropertyTypeFrom<typename PropertyValueT::ValueType>::type)
        , value(pv.value)
    {
    }

    template <IsPropertyValue PropertyValueT>
    explicit Property(PropertyValueT&& pv) noexcept(noexcept(std::is_nothrow_constructible_v<PropertyValueStorage, PropertyValueT&&>))
        : id(pv.id)
        , type(PropertyTypeFrom<typename PropertyValueT::ValueType>::type)
        , value(std::move(pv.value))
    {
    }

    template <SupportedPropertyType ValueT>
    explicit Property(PropId id, const ValueT& value) noexcept(noexcept(std::is_nothrow_constructible_v<PropertyValueStorage, const ValueT&>))
        : id(id)
        , type(PropertyTypeFrom<std::remove_cvref_t<ValueT>>::type)
        , value(value)
    {
    }

    template <SupportedPropertyType ValueT>
    explicit Property(PropId id, ValueT&& value) noexcept(noexcept(std::is_nothrow_constructible_v<PropertyValueStorage, ValueT&&>))
        : id(id)
        , type(PropertyTypeFrom<std::remove_cvref_t<ValueT>>::type)
        , value(std::move(value))
    {
    }

    friend void swap(Property& a, Property& b) noexcept(noexcept(std::is_nothrow_swappable_v<PropertyValueStorage>))
    {
        using std::swap;
        swap(a.id, b.id);
        swap(a.type, b.type);
        a.value.swap(b.value);
    }

    Property(const Property& o)
        : id(o.id)
        , type(o.type)
        , value(o.value)
    {
    }

    Property& operator=(const Property& o)
    {
        Property tmp(o);
        swap(*this, tmp);
        return *this;
    }

    Property(Property&& o) noexcept(noexcept(std::is_nothrow_move_constructible_v<PropertyValueStorage>))
        : id(o.id)
        , type(o.type)
        , value(std::move(o.value))
    {
        // make 'o' empty
        o.type = PropertyType::Invalid;
    }

    Property& operator=(Property&& o) noexcept(noexcept(std::is_nothrow_move_constructible_v<PropertyValueStorage>))
    {
        Property tmp(std::move(o));
        swap(*this, tmp);
        return *this;
    }

    constexpr bool empty() const noexcept
    {
        return type == PropertyType::Invalid;
    }

    friend auto operator==(const Property& a, const Property& b) noexcept
    {
        ErAssert(a.id == b.id);
        ErAssert(a.type == b.type);
        return a.value == b.value;
    }

    friend auto operator<=>(const Property& a, const Property& b) noexcept
    {
        ErAssert(a.id == b.id);
        ErAssert(a.type == b.type);
        return a.value <=> b.value;
    }

    const void* data() const noexcept
    {
        switch (type)
        {
        case PropertyType::Invalid: return nullptr;
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
        switch (type)
        {
        case PropertyType::Invalid: return 0;
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
        switch (type)
        {
        case PropertyType::Invalid: s << "<empty>"; break;
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

    PropId id = InvalidPropId;
    PropertyType type = PropertyType::Invalid;
    PropertyValueStorage value;
};


struct IPropertyInfo
{
    using Ptr = std::shared_ptr<IPropertyInfo>;

    virtual PropertyType type() const noexcept = 0;
    virtual PropId id() const noexcept = 0;
    virtual const char* id_str() const noexcept = 0;
    virtual const char* name() const noexcept = 0;
    virtual void format(const Property& v, std::ostream& s) const = 0;
    virtual bool equal(const Property& a, const Property& b) const noexcept = 0;
    virtual const void* data(const Property& p) const noexcept = 0;
    virtual std::size_t size(const Property& p) const noexcept = 0;

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
        if (v.type == PropertyType::Invalid)
        {
            s << "<\?\?\?>";
            return;
        }

        PropertyInfo::format(v.value, s);
    }

    bool equal(const Property& a, const Property& b) const noexcept override
    {
        if (a.type == PropertyType::Invalid || b.type == PropertyType::Invalid)
            return false;

        if (a.id != b.id)
            return false;

        ErAssert(a.type == b.type);

        return PropertyInfo::equal(a.value, b.value);
    }

    const void* data(const Property& p) const noexcept override
    {
        if (p.type == PropertyType::Invalid)
            return nullptr;

        return PropertyInfo::data(p.value);
    }

    std::size_t size(const Property& p) const noexcept override
    {
        if (p.type == PropertyType::Invalid)
            return 0;

        return PropertyInfo::size(p.value);
    }
};


} // namespace Er {}


#define ER_PROPID_(s)   Er::Util::crc32(s)
#define ER_PROPID(s)   Er::Util::crc32(s), s

