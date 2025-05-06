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

class ER_RTL_EXPORT Property
{
public:
    static constexpr std::size_t MaxNameLength = 32;
    using NameType = boost::static_string<MaxNameLength>;
    using MapType = std::map<NameType, Property, std::less<>>;
    using VectorType = std::vector<Property>;

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
        Map = 9,
        Vector = 10
    };

    static std::string_view typeToString(Type type) noexcept;
    
    constexpr Property() noexcept
        : m_name()
        , m_storage(Empty{})
        , m_semantics(Semantics::Default)
    {
    }

    constexpr Property(auto&& name, Bool v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v)
        , m_semantics(semantics)
    {
    }

    constexpr Property(auto&& name, bool v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v ? Er::True : Er::False)
        , m_semantics(semantics)
    {
    }

    constexpr Property(auto&& name, std::int32_t v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v)
        , m_semantics(semantics)
    {
    }

    constexpr Property(auto&& name, std::uint32_t v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v)
        , m_semantics(semantics)
    {
    }

    constexpr Property(auto&& name, std::int64_t v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v)
        , m_semantics(semantics)
    {
    }

    constexpr Property(auto&& name, std::uint64_t v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v)
        , m_semantics(semantics)
    {
    }

    constexpr Property(auto&& name, double v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v)
        , m_semantics(semantics)
    {
    }

    constexpr Property(auto&& name, const char* v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(std::string(v))
        , m_semantics(semantics)
    {
    }

    constexpr Property(auto&& name, std::string_view v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(std::string(v))
        , m_semantics(semantics)
    {
    }

    constexpr Property(auto&& name, const std::string& v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v)
        , m_semantics(semantics)
    {
    }

    constexpr Property(auto&& name, std::string&& v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(std::move(v))
        , m_semantics(semantics)
    {
    }

    constexpr Property(auto&& name, const Binary& v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v)
        , m_semantics(semantics)
    {
    }

    constexpr Property(auto&& name, Binary&& v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(std::move(v))
        , m_semantics(semantics)
    {
    }

    constexpr Property(auto&& name, const MapType& v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v)
        , m_semantics(semantics)
    {
    }

    constexpr Property(auto&& name, MapType&& v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(std::move(v))
        , m_semantics(semantics)
    {
    }

    constexpr Property(auto&& name, const VectorType& v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(v)
        , m_semantics(semantics)
    {
    }

    constexpr Property(auto&& name, VectorType&& v, SemanticCode semantics = Semantics::Default) noexcept
        : m_name(std::forward<decltype(name)>(name))
        , m_storage(std::move(v))
        , m_semantics(semantics)
    {
    }

    constexpr Property(const Property& o)
        : m_name(o.m_name)
        , m_storage(o.m_storage)
        , m_semantics(o.m_semantics)
    {
    }

    friend constexpr void swap(Property& o1, Property& o2) noexcept
    {
        o1.m_name.swap(o2.m_name);
        o1.m_storage.swap(o2.m_storage);
        std::swap(o1.m_semantics, o2.m_semantics);
    }

    constexpr Property& operator=(const Property& o)
    {
        Property tmp(o);
        swap(*this, tmp);
        return *this;
    }

    constexpr Property(Property&& o) noexcept
        : Property()
    {
        swap(*this, o);
    }

    constexpr Property& operator=(Property&& o) noexcept
    {
        Property tmp(std::move(o));
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

    [[nodiscard]] constexpr const NameType& name() const noexcept
    {
        return m_name;
    }

    [[nodiscard]] constexpr std::string_view nameStr() const noexcept
    {
        return { m_name.data(), m_name.length() };
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

    [[nodiscard]] constexpr MapType const* getMap() const noexcept
    {
        return std::get_if<MapType>(&m_storage);
    }

    [[nodiscard]] constexpr MapType* getMap() noexcept
    {
        return std::get_if<MapType>(&m_storage);
    }

    [[nodiscard]] constexpr VectorType const* getVector() const noexcept
    {
        return std::get_if<VectorType>(&m_storage);
    }

    [[nodiscard]] constexpr VectorType* getVector() noexcept
    {
        return std::get_if<VectorType>(&m_storage);
    }

    [[nodiscard]] bool operator==(const Property& other) const noexcept
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
        MapType,
        VectorType
    >;

    bool _eq(const Property& other) const noexcept;
    bool _eqEmpty(const Property& other) const noexcept;
    bool _eqBool(const Property& other) const noexcept;
    bool _eqInt32(const Property& other) const noexcept;
    bool _eqUInt32(const Property& other) const noexcept;
    bool _eqInt64(const Property& other) const noexcept;
    bool _eqUInt64(const Property& other) const noexcept;
    bool _eqDouble(const Property& other) const noexcept;
    bool _eqString(const Property& other) const noexcept;
    bool _eqBinary(const Property& other) const noexcept;
    bool _eqMap(const Property& other) const noexcept;
    bool _eqVector(const Property& other) const noexcept;

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
    std::string _strVector() const;

    NameType m_name;
    Storage m_storage;
    SemanticCode m_semantics;
};


template <class _Prop>
concept IsProperty = std::derived_from<_Prop, Property>;



template <typename T>
const T& get(const Property& v) noexcept;

template <>
[[nodiscard]] inline const Bool& get<>(const Property& v) noexcept
{
    ErAssert(v.type() == Property::Type::Bool);
    return *v.getBool();
}

template <>
[[nodiscard]] inline const int32_t& get<>(const Property& v) noexcept
{
    ErAssert(v.type() == Property::Type::Int32);
    return *v.getInt32();
}

template <>
[[nodiscard]] inline const uint32_t& get<>(const Property& v) noexcept
{
    ErAssert(v.type() == Property::Type::UInt32);
    return *v.getUInt32();
}

template <>
[[nodiscard]] inline const int64_t& get<>(const Property& v) noexcept
{
    ErAssert(v.type() == Property::Type::Int64);
    return *v.getInt64();
}

template <>
[[nodiscard]] inline const uint64_t& get<>(const Property& v) noexcept
{
    ErAssert(v.type() == Property::Type::UInt64);
    return *v.getUInt64();
}

template <>
[[nodiscard]] inline const double& get<>(const Property& v) noexcept
{
    ErAssert(v.type() == Property::Type::Double);
    return *v.getDouble();
}

template <>
[[nodiscard]] inline const std::string& get<>(const Property& v) noexcept
{
    ErAssert(v.type() == Property::Type::String);
    return *v.getString();
}

template <>
[[nodiscard]] inline const Binary& get<>(const Property& v) noexcept
{
    ErAssert(v.type() == Property::Type::Binary);
    return *v.getBinary();
}

template <>
[[nodiscard]] inline const Property::MapType& get<>(const Property& v) noexcept
{
    ErAssert(v.type() == Property::Type::Map);
    return *v.getMap();
}

template <>
[[nodiscard]] inline const Property::VectorType& get<>(const Property& v) noexcept
{
    ErAssert(v.type() == Property::Type::Vector);
    return *v.getVector();
}

[[nodiscard]] inline std::string formatProperty(const Property& prop)
{
    if (prop.empty()) [[unlikely]]
        return prop.str();

    auto& f = findPropertyFormatter(prop.semantics());
    return f(prop);
}


} // namespace Er {}