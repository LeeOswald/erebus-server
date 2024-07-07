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

    constexpr PropertyValue(const ValueT& value) noexcept(std::is_nothrow_copy_constructible_v<ValueT>)
        : value(value)
    {}

    constexpr PropertyValue(ValueT&& value) noexcept(std::is_nothrow_move_constructible_v<ValueT>)
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



struct EREBUS_EXPORT Property
{
    Property() noexcept = default;

    template <IsPropertyValue PropertyValueT>
    Property(const PropertyValueT& pv) noexcept(noexcept(std::is_nothrow_constructible_v<PropertyValueStorage, const PropertyValueT&>))
        : id(pv.id)
        , value(pv.value)
        , type(PropertyTypeFrom<typename PropertyValueT::ValueType>::type)
    {
        ErAssert(info || Er::lookupProperty(id));
    }

    template <IsPropertyValue PropertyValueT>
    Property(PropertyValueT&& pv) noexcept(noexcept(std::is_nothrow_constructible_v<PropertyValueStorage, PropertyValueT&&>))
        : id(pv.id)
        , value(std::move(pv.value))
        , type(PropertyTypeFrom<typename PropertyValueT::ValueType>::type)
    {
        ErAssert(info || Er::lookupProperty(id));
    }

    template <SupportedPropertyType ValueT>
    Property(PropId id, const ValueT& value, std::shared_ptr<IPropertyInfo> info = std::shared_ptr<IPropertyInfo>()) noexcept(noexcept(std::is_nothrow_constructible_v<PropertyValueStorage, const ValueT&>))
        : id(id)
        , value(value)
        , type(static_cast<PropertyType>(this->value.index()))
        , info(info)
    {
        ErAssert(info || Er::lookupProperty(id));
    }

    template <SupportedPropertyType ValueT>
    Property(PropId id, ValueT&& value, std::shared_ptr<IPropertyInfo> info = std::shared_ptr<IPropertyInfo>()) noexcept(noexcept(std::is_nothrow_constructible_v<PropertyValueStorage, ValueT&&>))
        : id(id)
        , value(std::move(value))
        , type(static_cast<PropertyType>(this->value.index()))
        , info(info)
    {
        ErAssert(info || Er::lookupProperty(id));
    }

    friend void swap(Property& a, Property& b) noexcept(noexcept(std::is_nothrow_swappable_v<PropertyValueStorage>))
    {
        using std::swap;
        swap(a.id, b.id);
        a.value.swap(b.value);
        swap(a.type, b.type);
        a.info.swap(b.info);
    }

    Property(const Property& o)
        : id(o.id)
        , value(o.value)
        , type(o.type)
        , info(o.info)
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
        , value(std::move(o.value))
        , type(o.type)
        , info(o.info)
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

    PropId id = InvalidPropId;
    PropertyValueStorage value;
    PropertyType type = PropertyType::Invalid;
    mutable std::shared_ptr<IPropertyInfo> info;
};


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
    void operator()(const T& v, std::ostream& s) { s << std::fixed << std::setprecision(3) << v; }
};

template <typename T>
struct PropertyFormatter<T, std::enable_if_t<std::is_same<T, std::string>::value>>
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

