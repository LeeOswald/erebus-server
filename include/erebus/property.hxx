#pragma once

#include <erebus/stringliteral.hxx>
#include <erebus/system/time.hxx>
#include <erebus/util/crc32.hxx>
#include <erebus/variant.hxx>


#include <iomanip>
#include <ostream>


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
    std::is_same_v<std::remove_cvref_t<T>, Binary> ||
    std::is_same_v<std::remove_cvref_t<T>, std::vector<bool>> ||
    std::is_same_v<std::remove_cvref_t<T>, std::vector<int32_t>> ||
    std::is_same_v<std::remove_cvref_t<T>, std::vector<uint32_t>> ||
    std::is_same_v<std::remove_cvref_t<T>, std::vector<int64_t>> ||
    std::is_same_v<std::remove_cvref_t<T>, std::vector<uint64_t>> ||
    std::is_same_v<std::remove_cvref_t<T>, std::vector<double>> ||
    std::is_same_v<std::remove_cvref_t<T>, std::vector<std::string>> ||
    std::is_same_v<std::remove_cvref_t<T>, std::vector<Binary>>;


using PropertyType = Variant::Type;

constexpr const char* propertyTypeToString(PropertyType type) noexcept
{
    switch (type)
    {
    case Er::PropertyType::Empty: return "Empty";
    case Er::PropertyType::Bool: return "Bool";
    case Er::PropertyType::Int32: return "Int32";
    case Er::PropertyType::UInt32: return "UInt32";
    case Er::PropertyType::Int64: return "Int64";
    case Er::PropertyType::UInt64: return "UInt64";
    case Er::PropertyType::Double: return "Double";
    case Er::PropertyType::String: return "String";
    case Er::PropertyType::Binary: return "Binary";
    case Er::PropertyType::Bools: return "Bool[]";
    case Er::PropertyType::Int32s: return "Int32[]";
    case Er::PropertyType::UInt32s: return "UInt32[]";
    case Er::PropertyType::Int64s: return "Int64[]";
    case Er::PropertyType::UInt64s: return "UInt64[]";
    case Er::PropertyType::Doubles: return "Double[]";
    case Er::PropertyType::Strings: return "String[]";
    case Er::PropertyType::Binaries: return "Binary[]";
    }

    return "<\?\?\?>";
}


struct Property;

struct IPropertyInfo
{
    using Ptr = std::shared_ptr<IPropertyInfo>;

    virtual ~IPropertyInfo() {}

    virtual PropertyType type() const noexcept = 0;
    virtual PropId id() const noexcept = 0;
    virtual const char* id_str() const noexcept = 0;
    virtual const char* name() const noexcept = 0;
    virtual void format(const Property& v, std::ostream& s) const = 0;
};

EREBUS_EXPORT std::shared_ptr<IPropertyInfo> lookupProperty(std::string_view domain, PropId id) noexcept;
EREBUS_EXPORT std::shared_ptr<IPropertyInfo> lookupProperty(std::string_view domain, const char* id) noexcept;


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
struct PropertyTypeFrom<Binary>
{
    static constexpr PropertyType type = PropertyType::Binary;
};


template <SupportedPropertyType ValueT, PropId PrId, StringLiteral PrIdStr, StringLiteral PrName, class FormatterT = PropertyFormatter<ValueT>>
struct PropertyInfo
    : public IPropertyInfo
{
    using ValueType = std::decay_t<ValueT>;
    using Id = std::integral_constant<PropId, PrId>;
    using Formatter = FormatterT;

    PropertyType type() const noexcept override
    {
        return PropertyTypeFrom<ValueType>::type;
    }

    PropId id() const noexcept override
    {
        return Id::value;
    }

    const char* id_str() const noexcept override
    {
        return fromStringLiteral<PrIdStr>();
    }

    const char* name() const noexcept override
    {
        return fromStringLiteral<PrName>();
    }

    void format(const Property& v, std::ostream& s) const override;
};


struct PropertyValueTag
{
};

template <SupportedPropertyType ValueT, PropId PrId, StringLiteral PrIdStr, StringLiteral PrName, class FormatterT = PropertyFormatter<ValueT>>
struct PropertyValue final
{
    using Tag = PropertyValueTag;
    using Info = PropertyInfo<ValueT, PrId, PrIdStr, PrName, FormatterT>;
    using ValueType = ValueT;
    using Id = std::integral_constant<PropId, PrId>;

    static std::shared_ptr<IPropertyInfo> make_info()
    {
        return std::make_shared<Info>();
    }

    PropertyValue() noexcept(noexcept(std::is_nothrow_constructible_v<ValueT>))
        : m_value()
    {}

    constexpr explicit PropertyValue(const ValueT& value) noexcept(std::is_nothrow_copy_constructible_v<ValueT>)
        : m_value(value)
    {}

    constexpr explicit PropertyValue(ValueT&& value) noexcept(std::is_nothrow_move_constructible_v<ValueT>)
        : m_value(std::move(value))
    {}

    static PropertyType type() noexcept
    {
        return PropertyTypeFrom<typename Info::ValueType>::type;
    }

    static PropId id() noexcept
    {
        return Info::Id::value;
    }

    static const char* id_str() noexcept
    {
        return fromStringLiteral<PrIdStr>();
    }

    static const char* name() noexcept
    {
        return fromStringLiteral<PrName>();
    }

    const ValueType& value() const noexcept
    {
        return m_value;
    }

    ValueType& value() noexcept
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
    requires std::same_as<typename T::Tag, PropertyValueTag>;
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
struct PropertyFormatter<T, std::enable_if_t<std::is_same<T, Binary>::value>>
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
    Property() noexcept
        : value()
        , id(InvalidPropId)
    {
    }

    template <IsPropertyValue PropertyValueT>
    constexpr Property(const PropertyValueT& pv)
        : value(pv.value())
        , id(pv.id())
    {
    }

    template <IsPropertyValue PropertyValueT>
    constexpr Property(PropertyValueT&& pv) noexcept
        : value(std::move(pv.value()))
        , id(pv.id())
    {
    }

    template <SupportedPropertyType ValueT>
    constexpr explicit Property(PropId id, const ValueT& value)
        : value(value)
        , id(id)
    {
    }

    template <SupportedPropertyType ValueT>
    constexpr explicit Property(PropId id, ValueT&& value) noexcept
        : value(std::move(value))
        , id(id)
    {
    }

    constexpr void swap(Property& a, Property& b) noexcept
    {
        std::swap(a.id, b.id);
        a.value.swap(b.value);
    }

    constexpr Property(const Property& o)
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

    constexpr Property(Property&& o) noexcept
        : value(std::move(o.value))
        , id(o.id)
    {
    }

    Property& operator=(Property&& o) noexcept
    {
        Property tmp(std::move(o));
        swap(*this, tmp);
        return *this;
    }

    constexpr PropertyType type() const noexcept
    {
        return value.type();
    }

    constexpr bool empty() const noexcept
    {
        return value.empty();
    }

    constexpr bool operator==(const Property& other) const noexcept
    {
        return value == other.value;
    }

    void format(std::ostream& s) const
    {
        switch (type())
        {
        case PropertyType::Empty: s << "<empty>"; break;
        case PropertyType::Bool: PropertyFormatter<bool>()(get<bool>(value), s); break;
        case PropertyType::Int32: PropertyFormatter<int32_t>()(get<int32_t>(value), s); break;
        case PropertyType::UInt32: PropertyFormatter<uint32_t>()(get<uint32_t>(value), s); break;
        case PropertyType::Int64: PropertyFormatter<int64_t>()(get<int64_t>(value), s); break;
        case PropertyType::UInt64: PropertyFormatter<uint64_t>()(get<uint64_t>(value), s); break;
        case PropertyType::Double: PropertyFormatter<double>()(get<double>(value), s); break;
        case PropertyType::String: PropertyFormatter<std::string>()(get<std::string>(value), s); break;
        case PropertyType::Binary: PropertyFormatter<Binary>()(get<Binary>(value), s); break;
        default: s << "<\?\?\?>";
        }
    }

    Variant value;
    PropId id;
};


template <SupportedPropertyType ValueT, PropId PrId, StringLiteral PrIdStr, StringLiteral PrName, class FormatterT>
void PropertyInfo<ValueT, PrId, PrIdStr, PrName, FormatterT>::format(const Property& v, std::ostream& s) const
{
    if (PropertyTypeFrom<ValueType>::type == v.type()) [[likely]]
    {
        auto const& val = get<ValueType>(v.value);
        Formatter f;
        f(val, s);
    }
    else
    {
        s << "<bad property cast>";
        return;
    }
}

} // namespace Er {}


#define ER_PROPID_(s)   Er::Util::crc32(s)
#define ER_PROPID(s)   Er::Util::crc32(s), s

