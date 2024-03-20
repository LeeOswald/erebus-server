#pragma once

#include <erebus/stringliteral.hxx>
#include <erebus/system/time.hxx>
#include <erebus/util/crc32.hxx>

#include <any>
#include <iomanip>
#include <ostream>
#include <typeinfo>
#include <unordered_map>


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
    std::is_same_v<T, bool> ||
    std::is_same_v<T, int32_t> ||
    std::is_same_v<T, uint32_t> ||
    std::is_same_v<T, int64_t> ||
    std::is_same_v<T, uint64_t> ||
    std::is_same_v<T, double> ||
    std::is_same_v<T, std::string> ||
    std::is_same_v<T, int32_t> ||
    std::is_same_v<T, Bytes>;


template <SupportedPropertyType ValueT, PropId PrId, StringLiteral PrIdStr, StringLiteral PrName, class ComparatorT = PropertyComparator<ValueT>, class FormatterT = PropertyFormatter<ValueT>>
class PropertyInfo
{
public:
    using ValueType = ValueT;
    using Id = std::integral_constant<PropId, PrId>;
    using Comparator = ComparatorT;
    using Formatter = FormatterT;

    static constexpr const std::type_info& type() noexcept
    {
        return typeid(ValueT);
    }

    static constexpr PropId id() noexcept
    {
        return Id::value;
    }

    static constexpr const char* idstr() noexcept
    {
        return fromStringLiteral<PrIdStr>();
    }
    
    static constexpr const char* name() noexcept
    {
        return fromStringLiteral<PrName>();
    }
};


template <SupportedPropertyType ValueT, PropId PrId, StringLiteral PrIdStr, StringLiteral PrName, class ComparatorT = PropertyComparator<ValueT>, class FormatterT = PropertyFormatter<ValueT>>
class PropertyValue final
    : public PropertyInfo<ValueT, PrId, PrIdStr, PrName, ComparatorT, FormatterT>
{
public:
    using Base = PropertyInfo<ValueT, PrId, PrIdStr, PrName, ComparatorT, FormatterT>;
    using ValueType = Base::ValueType;
    using Id = Base::Id;
    using Comparator = ComparatorT;
    using Formatter = Base::Formatter;

    PropertyValue() noexcept = default;

    constexpr PropertyValue(const ValueT& value)
        : m_value(value)
    {}

    constexpr PropertyValue(ValueT&& value) noexcept
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


struct IPropertyInfo;

struct Property
{
    Property() = default;

    Property(PropId id, std::any&& value, IPropertyInfo* info = nullptr) noexcept
        : id(id)
        , value(std::move(value))
        , info(info)
    {}

    template <typename ValueT>
    Property(PropId id, ValueT&& value, IPropertyInfo* info = nullptr) noexcept
        : id(id)
        , value(std::forward<ValueT>(value))
        , info(info)
    {}

    PropId id = InvalidPropId;
    std::any value;
    mutable IPropertyInfo* info = nullptr;
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
    bool operator()(const Property& a, const Property& b) { return std::any_cast<Bytes>(a.value) == std::any_cast<Bytes>(b.value); }
};

template <std::equality_comparable T>
struct PropertyComparator<T>
{
    bool operator()(const Property& a, const Property& b) { return std::any_cast<T>(a.value) == std::any_cast<T>(b.value); }
};


struct NullPropertyFormatter
{
    void operator()([[maybe_unused]] const Property& v, [[maybe_unused]] std::ostream& s) {  }
};

template <typename T>
struct PropertyFormatter<T, std::enable_if_t<std::is_same<T, bool>::value>>
{
    void operator()(const Property& v, std::ostream& s) { s << std::boolalpha << std::any_cast<T>(v.value); }
};

template <typename T>
struct PropertyFormatter<T, std::enable_if_t<std::is_integral<T>::value && !std::is_same<T, bool>::value>>
{
    void operator()(const Property& v, std::ostream& s) { s << std::any_cast<T>(v.value); }
};

template <typename T>
struct PropertyFormatter<T, std::enable_if_t<std::is_same<T, std::string>::value>>
{
    void operator()(const Property& v, std::ostream& s) { s << std::any_cast<T>(v.value); }
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
        auto packed = std::any_cast<uint64_t>(v.value);
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

    virtual const std::type_info& type() const noexcept = 0;
    virtual PropId id() const noexcept = 0;
    virtual const char* idstr() const noexcept = 0;
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

    const std::type_info& type() const noexcept override
    {
        return PropertyInfo::type();
    }

    PropId id() const noexcept override
    {
        return PropertyInfo::id();
    }

    const char* idstr() const noexcept override
    {
        return PropertyInfo::idstr();
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

template <typename T>
std::optional<T> getProperty(const PropertyBag& bag, PropId id)
{
    auto it = bag.find(id);
    if (it == bag.end())
        return std::nullopt;

    return std::make_optional<T>(std::any_cast<T>(it->second.value));
}

template <typename T>
T getProperty(const PropertyBag& bag, PropId id, T&& defaultValue)
{
    auto it = bag.find(id);
    if (it == bag.end())
        return T(std::forward<T>(defaultValue));

    return std::any_cast<T>(it->second.value);
}


} // namespace Er {}


#define ER_PROPID_(s)   Er::Util::crc32(s)
#define ER_PROPID(s)   Er::Util::crc32(s), s

