#pragma once

#include <erebus/stringliteral.hxx>
#include <erebus/util/crc32.hxx>

#include <any>
#include <ostream>
#include <typeinfo>
#include <vector>


namespace Er
{

using PropId = uint32_t;
constexpr PropId InvalidPropId = PropId(-1);


template <typename ValueT, PropId PrId, StringLiteral PrIdStr, StringLiteral PrName, class FormatterT, typename BaseT = ValueT>
class PropertyInfo final
{
public:
    using ValueType = ValueT;
    using BaseType = BaseT;
    using Id = std::integral_constant<PropId, PrId>;
    using Formatter = FormatterT;

    PropertyInfo() noexcept = default;

    template <typename T>
    explicit constexpr PropertyInfo(T&& value) noexcept
        : m_value(std::forward<T>(value))
    {}

    static constexpr const std::type_info& type() noexcept
    {
        return typeid(ValueT);
    }

    static constexpr const std::type_info& base() noexcept
    {
        return typeid(BaseT);
    }

    static constexpr PropId id() noexcept
    {
        return Id::value;
    }

    static constexpr const char* idstr() noexcept
    {
        return fromStringLiteral<PrIdStr>();
    }
    
    constexpr ValueType&& value() && noexcept
    {
        return std::move(m_value);
    }

    constexpr ValueType& value() & noexcept
    {
        return m_value;
    }

    static constexpr const char* name() noexcept
    {
        return fromStringLiteral<PrName>();
    }

    void format(std::ostream& strem) const
    {
        Formatter f;
        f(m_value, strem);
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
    IPropertyInfo* info = nullptr;
};


template <typename T, typename = void>
struct PropertyFormatter;

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


struct IPropertyInfo
{
    using Ptr = std::shared_ptr<IPropertyInfo>;

    virtual const std::type_info& type() const = 0;
    virtual const std::type_info& base() const = 0;
    virtual PropId id() const = 0;
    virtual const char* idstr() const = 0;
    virtual const char* name() const = 0;
    virtual void format(const Property& v, std::ostream& s) = 0;

protected:
    virtual ~IPropertyInfo() {}
};


template <class PropertyInfoT>
struct PropertyInfoWrapper
    : public IPropertyInfo
{
    using PropertyInfo = PropertyInfoT;
    using Formatter = typename PropertyInfo::Formatter;

    const std::type_info& type() const override
    {
        return PropertyInfo::type();
    }

    const std::type_info& base() const override
    {
        return PropertyInfo::base();
    }

    PropId id() const override
    {
        return PropertyInfo::id();
    }

    const char* idstr() const override
    {
        return PropertyInfo::idstr();
    }

    const char* name() const override
    {
        return PropertyInfo::name();
    }

    void format(const Property& v, std::ostream& s) override
    {
        Formatter f;
        f(v, s);
    }
};


} // namespace Er {}


#define ER_PROPID_(s)   Er::Util::crc32(s)
#define ER_PROPID(s)   Er::Util::crc32(s), s

