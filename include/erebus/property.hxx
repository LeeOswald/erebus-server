#pragma once

#include <erebus/stringliteral.hxx>
#include <erebus/system/packed_time.hxx>
#include <erebus/util/crc32.hxx>
#include <erebus/variant.hxx>


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
    std::is_same_v<std::remove_cvref_t<T>, Bool> ||
    std::is_same_v<std::remove_cvref_t<T>, int32_t> ||
    std::is_same_v<std::remove_cvref_t<T>, uint32_t> ||
    std::is_same_v<std::remove_cvref_t<T>, int64_t> ||
    std::is_same_v<std::remove_cvref_t<T>, uint64_t> ||
    std::is_same_v<std::remove_cvref_t<T>, double> ||
    std::is_same_v<std::remove_cvref_t<T>, std::string> ||
    std::is_same_v<std::remove_cvref_t<T>, Binary> ||
    std::is_same_v<std::remove_cvref_t<T>, std::vector<Bool>> ||
    std::is_same_v<std::remove_cvref_t<T>, std::vector<int32_t>> ||
    std::is_same_v<std::remove_cvref_t<T>, std::vector<uint32_t>> ||
    std::is_same_v<std::remove_cvref_t<T>, std::vector<int64_t>> ||
    std::is_same_v<std::remove_cvref_t<T>, std::vector<uint64_t>> ||
    std::is_same_v<std::remove_cvref_t<T>, std::vector<double>> ||
    std::is_same_v<std::remove_cvref_t<T>, std::vector<std::string>> ||
    std::is_same_v<std::remove_cvref_t<T>, std::vector<Binary>>;


using PropertyType = Variant::Type;

using BoolVector = Variant::BoolVector;
using Int32Vector = Variant::Int32Vector;
using UInt32Vector = Variant::UInt32Vector;
using Int64Vector = Variant::Int64Vector;
using UInt64Vector = Variant::UInt64Vector;
using DoubleVector = Variant::DoubleVector;
using StringVector = Variant::StringVector;
using BinaryVector = Variant::BinaryVector;


using PropertyRef = std::variant<
    Bool*,
    int32_t*,
    uint32_t*,
    int64_t*,
    uint64_t*,
    double*,
    std::string*,
    Binary*,
    std::vector<Bool>*,
    std::vector<int32_t>*,
    std::vector<uint32_t>*,
    std::vector<int64_t>*,
    std::vector<uint64_t>*,
    std::vector<double>*,
    std::vector<std::string>*,
    std::vector<Binary>*
>;

using ConstPropertyRef = std::variant<
    const Bool*,
    const int32_t*,
    const uint32_t*,
    const int64_t*,
    const uint64_t*,
    const double*,
    const std::string*,
    const Binary*,
    const std::vector<Bool>*,
    const std::vector<int32_t>*,
    const std::vector<uint32_t>*,
    const std::vector<int64_t>*,
    const std::vector<uint64_t>*,
    const std::vector<double>*,
    const std::vector<std::string>*,
    const std::vector<Binary>*
>;

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

constexpr bool propertyTypeIsArray(PropertyType type) noexcept
{
    if (type >= Er::PropertyType::Bools && type <= Er::PropertyType::Binaries)
        return true;
    
    return false;
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
    virtual std::string to_string(const Property& v) const = 0;
};

EREBUS_EXPORT std::shared_ptr<IPropertyInfo> lookupProperty(std::string_view domain, PropId id) noexcept;
EREBUS_EXPORT std::shared_ptr<IPropertyInfo> lookupProperty(std::string_view domain, const char* id) noexcept;


template <typename T>
struct PropertyTypeFrom;

template <>
struct PropertyTypeFrom<Bool>
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

template <>
struct PropertyTypeFrom<BoolVector>
{
    static constexpr PropertyType type = PropertyType::Bools;
};

template <>
struct PropertyTypeFrom<Int32Vector>
{
    static constexpr PropertyType type = PropertyType::Int32s;
};

template <>
struct PropertyTypeFrom<UInt32Vector>
{
    static constexpr PropertyType type = PropertyType::UInt32s;
};

template <>
struct PropertyTypeFrom<Int64Vector>
{
    static constexpr PropertyType type = PropertyType::Int64s;
};

template <>
struct PropertyTypeFrom<UInt64Vector>
{
    static constexpr PropertyType type = PropertyType::UInt64s;
};

template <>
struct PropertyTypeFrom<DoubleVector>
{
    static constexpr PropertyType type = PropertyType::Doubles;
};

template <>
struct PropertyTypeFrom<StringVector>
{
    static constexpr PropertyType type = PropertyType::Strings;
};

template <>
struct PropertyTypeFrom<BinaryVector>
{
    static constexpr PropertyType type = PropertyType::Binaries;
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

    std::string to_string(const Property& v) const override;
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
    std::string operator()([[maybe_unused]] ConstPropertyRef v) 
    {
        return std::string();
    }
};

template <typename T>
struct PropertyFormatter<T, std::enable_if_t<std::is_same<T, Bool>::value>>
{
    std::string operator()(ConstPropertyRef v) 
    {
        auto pp = std::get_if<const T*>(&v);
        if (!pp || !*pp)
            return std::string("<null>");

        return (**pp == True) ? "True" : "False";
    }
};

template <typename T>
struct PropertyFormatter<T, std::enable_if_t<std::is_integral<T>::value && !std::is_same<T, Bool>::value>>
{
    std::string operator()(ConstPropertyRef v) 
    {
        auto pp = std::get_if<const T*>(&v);
        if (!pp || !*pp)
            return std::string("<null>");

        return std::to_string(**pp);
    }
};

template <typename T>
struct PropertyFormatter<T, std::enable_if_t<std::is_floating_point<T>::value>>
{
    explicit PropertyFormatter(int precision = 3)
        : precision(precision)
    {}

    std::string operator()(ConstPropertyRef v) 
    {
        auto pp = std::get_if<const T*>(&v);
        if (!pp || !*pp)
            return std::string("<null>");

        std::ostringstream ss;
        ss << std::fixed << std::setprecision(precision) << **pp;
        return ss.str();
    }

    int precision;
};

template <typename T>
struct PropertyFormatter<T, std::enable_if_t<std::is_same<T, std::string>::value>>
{
    std::string operator()(ConstPropertyRef v) 
    {
        auto pp = std::get_if<const T*>(&v);
        if (!pp || !*pp)
            return std::string("<null>");

        return **pp;
    }
};

template <typename T>
struct PropertyFormatter<T, std::enable_if_t<std::is_same<T, Binary>::value>>
{
    std::string operator()(ConstPropertyRef v) 
    {
        auto pp = std::get_if<const T*>(&v);
        if (!pp || !*pp)
            return std::string("<null>");

        std::ostringstream ss;
        ss << **pp;
        return ss.str();
    }
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

    std::string operator()(ConstPropertyRef v) 
    {
        auto pp = std::get_if<const uint64_t*>(&v);
        if (!pp || !*pp)
            return std::string("<null>");

        struct tm t = {};
        Er::System::PackedTime packed(**pp);
        if constexpr (Zone == TimeZone::Utc)
            t = packed.toUtc();
        else
            t = packed.toLocalTime();

        std::ostringstream ss;
        ss << std::put_time(&t, format);
        return ss.str();
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

    std::string to_string() const
    {
        switch (type())
        {
        case PropertyType::Empty: 
            return std::string("[empty]");
        case PropertyType::Bool: 
            return PropertyFormatter<Bool>()(ConstPropertyRef(&get<Bool>(value)));
        case PropertyType::Int32: 
            return PropertyFormatter<int32_t>()(ConstPropertyRef(&get<int32_t>(value)));
        case PropertyType::UInt32: 
            return PropertyFormatter<uint32_t>()(ConstPropertyRef(&get<uint32_t>(value)));
        case PropertyType::Int64: 
            return PropertyFormatter<int64_t>()(ConstPropertyRef(&get<int64_t>(value)));
        case PropertyType::UInt64: 
            return PropertyFormatter<uint64_t>()(ConstPropertyRef(&get<uint64_t>(value)));
        case PropertyType::Double: 
            return PropertyFormatter<double>()(ConstPropertyRef(&get<double>(value)));
        case PropertyType::String: 
            return PropertyFormatter<std::string>()(ConstPropertyRef(&get<std::string>(value)));
        case PropertyType::Binary: 
            return PropertyFormatter<Binary>()(ConstPropertyRef(&get<Binary>(value)));
        case PropertyType::Bools:
            return vectorToString<Bool>(get<BoolVector>(value));
        case PropertyType::Int32s:
            return vectorToString<int32_t>(get<Int32Vector>(value));
        case PropertyType::UInt32s:
            return vectorToString<uint32_t>(get<UInt32Vector>(value));
        case PropertyType::Int64s:
            return vectorToString<int64_t>(get<Int64Vector>(value));
        case PropertyType::UInt64s:
            return vectorToString<uint64_t>(get<UInt64Vector>(value));
        case PropertyType::Doubles:
            return vectorToString<double>(get<DoubleVector>(value));
        case PropertyType::Strings:
            return vectorToString<std::string>(get<StringVector>(value));
        case PropertyType::Binaries:
            return vectorToString<Binary>(get<BinaryVector>(value));
        }

        return std::string("<\?\?\?>");
    }

    Variant value;
    PropId id;

private:
    template <typename T>
    static std::string vectorToString(const std::vector<T>& arr)
    {
        if (arr.empty())
            return std::string("[]");

        PropertyFormatter<T> f;

        std::ostringstream ss;
        ss << "[ ";
        bool first = true;
        for (auto& v: arr)
        {
            if (first)
                first = false;
            else
                ss << ", ";

            if constexpr (std::is_same_v<T, std::string>)
                ss << "\"";
            else if constexpr (std::is_same_v<T, Binary>)
                ss << "[";

            ss << f(&v);

            if constexpr (std::is_same_v<T, std::string>)
                ss << "\"";
            else if constexpr (std::is_same_v<T, Binary>)
                ss << "]";

        }
        ss << " ]";

        return ss.str();
    }
};


namespace __
{

template <typename T, typename FormatterT>
static std::string vectorToString(const std::vector<T>& arr, FormatterT& f)
{
    if (arr.empty())
        return std::string("[]");

    std::ostringstream ss;
    ss << "[ ";
    bool first = true;
    for (auto& v: arr)
    {
        if (first)
            first = false;
        else
            ss << ", ";

        if constexpr (std::is_same_v<T, std::string>)
            ss << "\"";
        else if constexpr (std::is_same_v<T, Binary>)
            ss << "[";

        ss << f(&v);

        if constexpr (std::is_same_v<T, std::string>)
            ss << "\"";
        else if constexpr (std::is_same_v<T, Binary>)
            ss << "]";

    }
    ss << " ]";
    return ss.str();
} 

} // namespace __ {}



template <SupportedPropertyType ValueT, PropId PrId, StringLiteral PrIdStr, StringLiteral PrName, class FormatterT>
std::string PropertyInfo<ValueT, PrId, PrIdStr, PrName, FormatterT>::to_string(const Property& v) const
{
    if (v.empty()) [[unlikely]]
        return std::string("<empty>");

    if (PropertyTypeFrom<ValueType>::type != v.type()) [[unlikely]]
        return std::string("<bad property cast>");

    Formatter f;
    
    if constexpr (!propertyTypeIsArray(PropertyTypeFrom<ValueType>::type))
        return f(&get<ValueType>(v.value));
    else 
        return __::vectorToString<typename ValueType::value_type>(get<ValueType>(v.value), f);

}

} // namespace Er {}


#define ER_PROPID_(s)   Er::Util::crc32(s)
#define ER_PROPID(s)   Er::Util::crc32(s), s

