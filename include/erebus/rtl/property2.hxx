#pragma once

#include <erebus/rtl/binary.hxx>
#include <erebus/rtl/empty.hxx>
#include <erebus/rtl/property_format.hxx>

#include <map>
#include <variant>
#include <vector>

#include <boost/static_string/static_string.hpp>


namespace Er
{

class ER_RTL_EXPORT Property2 final
{
public:
    static constexpr std::size_t MaxNameLength = 32;
    using Name = boost::static_string<MaxNameLength>;
    using Map = std::map<Name, Property2>;

    enum class Type: std::uint32_t
    {
        Empty = 0,
        Bool = 1,
        Int32 = 2,
        UInt32 = 3,
        Int64 = 4,
        UInt64 = 5,
        Double = 6,
        String = 7,
        Binary = 8,
        Map = 9
    };

    static std::string_view typeToString(Type type) noexcept;
    
    constexpr Property2() noexcept = default;

    constexpr Property2(auto&& name, Bool v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v)
        , m_semantics(semantics)
    {
    }

    constexpr Property2(auto&& name, bool v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v ? Er::True : Er::False)
        , m_semantics(semantics)
    {
    }

    constexpr Property2(auto&& name, std::int32_t v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v)
        , m_semantics(semantics)
    {
    }

    constexpr Property2(auto&& name, std::uint32_t v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v)
        , m_semantics(semantics)
    {
    }

    constexpr Property2(auto&& name, std::int64_t v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v)
        , m_semantics(semantics)
    {
    }

    constexpr Property2(auto&& name, std::uint64_t v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v)
        , m_semantics(semantics)
    {
    }

    constexpr Property2(auto&& name, double v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v)
        , m_semantics(semantics)
    {
    }

    constexpr Property2(auto&& name, const char* v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(std::string(v))
        , m_semantics(semantics)
    {
    }

    constexpr Property2(auto&& name, std::string_view v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(std::string(v))
        , m_semantics(semantics)
    {
    }

    constexpr Property2(auto&& name, const std::string& v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v)
        , m_semantics(semantics)
    {
    }

    constexpr Property2(auto&& name, std::string&& v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(std::move(v))
        , m_semantics(semantics)
    {
    }

    constexpr Property2(auto&& name, const Binary& v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v)
        , m_semantics(semantics)
    {
    }

    constexpr Property2(auto&& name, Binary&& v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(std::move(v))
        , m_semantics(semantics)
    {
    }

    constexpr Property2(auto&& name, const Map& v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v)
        , m_semantics(semantics)
    {
    }

    constexpr Property2(auto&& name, Map&& v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(std::move(v))
        , m_semantics(semantics)
    {
    }

    constexpr Property2(const Property2& o)
        : m_name(o.m_name)
        , m_storage(o.m_storage)
        , m_semantics(o.m_semantics)
    {
    }

    friend constexpr void swap(Property2& o1, Property2& o2) noexcept
    {
        o1.m_name.swap(o2.m_name);
        o1.m_storage.swap(o2.m_storage);
        std::swap(o1.m_semantics, o2.m_semantics);
    }

    constexpr Property2& operator=(const Property2& o)
    {
        Property2 tmp(o);
        swap(*this, tmp);
        return *this;
    }

    constexpr Property2(Property2&& o) noexcept
        : Property2()
    {
        swap(*this, o);
    }

    constexpr Property2& operator=(Property2&& o) noexcept
    {
        Property2 tmp(std::move(o));
        swap(*this, tmp);
        return *this;
    }

    [[nodiscard]] constexpr Type type() const noexcept
    {
        return static_cast<Type>(m_storage.index());
    }

    [[nodiscard]] constexpr SemanticCode semantics() const noexcept
    {
        return m_semantics;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return type() == Type::Empty;
    }

    [[nodiscard]] constexpr std::string_view name() const noexcept
    {
        return std::string_view{ m_name.data(), m_name.length() };
    }

    [[nodiscard]] constexpr Bool const* getBool() const noexcept
    {
        return std::get_if<Bool>(&m_storage);
    }

    [[nodiscard]] constexpr Bool* getBool() noexcept
    {
        return std::get_if<Bool>(&m_storage);
    }

    [[nodiscard]] constexpr std::int32_t const* getInt32() const noexcept
    {
        return std::get_if<std::int32_t>(&m_storage);
    }

    [[nodiscard]] constexpr std::int32_t* getInt32() noexcept
    {
        return std::get_if<std::int32_t>(&m_storage);
    }

    [[nodiscard]] constexpr std::uint32_t const* getUInt32() const noexcept
    {
        return std::get_if<std::uint32_t>(&m_storage);
    }

    [[nodiscard]] constexpr std::uint32_t* getUInt32() noexcept
    {
        return std::get_if<std::uint32_t>(&m_storage);
    }

    [[nodiscard]] constexpr std::int64_t const* getInt64() const noexcept
    {
        return std::get_if<std::int64_t>(&m_storage);
    }

    [[nodiscard]] constexpr std::int64_t* getInt64() noexcept
    {
        return std::get_if<std::int64_t>(&m_storage);
    }

    [[nodiscard]] constexpr std::uint64_t const* getUInt64() const noexcept
    {
        return std::get_if<std::uint64_t>(&m_storage);
    }

    [[nodiscard]] constexpr std::uint64_t* getUInt64() noexcept
    {
        return std::get_if<std::uint64_t>(&m_storage);
    }

    [[nodiscard]] constexpr double const* getDouble() const noexcept
    {
        return std::get_if<double>(&m_storage);
    }

    [[nodiscard]] constexpr double* getDouble() noexcept
    {
        return std::get_if<double>(&m_storage);
    }

    [[nodiscard]] constexpr std::string const* getString() const noexcept
    {
        return std::get_if<std::string>(&m_storage);
    }

    [[nodiscard]] constexpr std::string* getString() noexcept
    {
        return std::get_if<std::string>(&m_storage);
    }

    [[nodiscard]] constexpr Binary const* getBinary() const noexcept
    {
        return std::get_if<Binary>(&m_storage);
    }

    [[nodiscard]] constexpr Binary* getBinary() noexcept
    {
        return std::get_if<Binary>(&m_storage);
    }

    [[nodiscard]] constexpr Map const* getMap() const noexcept
    {
        return std::get_if<Map>(&m_storage);
    }

    [[nodiscard]] constexpr Map* getMap() noexcept
    {
        return std::get_if<Map>(&m_storage);
    }

    [[nodiscard]] bool operator==(const Property2& other) const noexcept
    {
        return _eq(other);
    }

    [[nodiscard]] std::string str() const
    {
        return _str();
    }

private:
    using Storage = std::variant<
        Empty,
        Bool,
        std::int32_t,
        std::uint32_t,
        std::int64_t,
        std::uint64_t,
        double,
        std::string,
        Binary,
        Map
    >;

    bool _eq(const Property2& other) const noexcept;
    bool _eqEmpty(const Property2& other) const noexcept;
    bool _eqBool(const Property2& other) const noexcept;
    bool _eqInt32(const Property2& other) const noexcept;
    bool _eqUInt32(const Property2& other) const noexcept;
    bool _eqInt64(const Property2& other) const noexcept;
    bool _eqUInt64(const Property2& other) const noexcept;
    bool _eqDouble(const Property2& other) const noexcept;
    bool _eqString(const Property2& other) const noexcept;
    bool _eqBinary(const Property2& other) const noexcept;
    bool _eqMap(const Property2& other) const noexcept;

    std::string _str() const;
    std::string _strEmpty() const;
    std::string _strBool() const;
    std::string _strInt32() const;
    std::string _strUInt32() const;
    std::string _strInt64() const;
    std::string _strUInt64() const;
    std::string _strDouble() const;
    std::string _strString() const;
    std::string _strBinary() const;
    std::string _strMap() const;

    Name m_name;
    Storage m_storage;
    SemanticCode m_semantics;
};


template <typename T>
const T& get(const Property2& v) noexcept;

template <>
[[nodiscard]] inline const Bool& get<>(const Property2& v) noexcept
{
    ErAssert(v.type() == Property2::Type::Bool);
    return *v.getBool();
}

template <>
[[nodiscard]] inline const int32_t& get<>(const Property2& v) noexcept
{
    ErAssert(v.type() == Property2::Type::Int32);
    return *v.getInt32();
}

template <>
[[nodiscard]] inline const uint32_t& get<>(const Property2& v) noexcept
{
    ErAssert(v.type() == Property2::Type::UInt32);
    return *v.getUInt32();
}

template <>
[[nodiscard]] inline const int64_t& get<>(const Property2& v) noexcept
{
    ErAssert(v.type() == Property2::Type::Int64);
    return *v.getInt64();
}

template <>
[[nodiscard]] inline const uint64_t& get<>(const Property2& v) noexcept
{
    ErAssert(v.type() == Property2::Type::UInt64);
    return *v.getUInt64();
}

template <>
[[nodiscard]] inline const double& get<>(const Property2& v) noexcept
{
    ErAssert(v.type() == Property2::Type::Double);
    return *v.getDouble();
}

template <>
[[nodiscard]] inline const std::string& get<>(const Property2& v) noexcept
{
    ErAssert(v.type() == Property2::Type::String);
    return *v.getString();
}

template <>
[[nodiscard]] inline const Binary& get<>(const Property2& v) noexcept
{
    ErAssert(v.type() == Property2::Type::Binary);
    return *v.getBinary();
}

template <>
[[nodiscard]] inline const Property2::Map& get<>(const Property2& v) noexcept
{
    ErAssert(v.type() == Property2::Type::Map);
    return *v.getMap();
}

[[nodiscard]] inline std::string formatProperty(const Property2& prop)
{
    if (prop.empty()) [[unlikely]]
        return prop.str();

    auto& f = findPropertyFormatter(prop.semantics());
    return f(prop);
}


using PropertyBag2 = std::vector<Property2>;

} // namespace Er {}