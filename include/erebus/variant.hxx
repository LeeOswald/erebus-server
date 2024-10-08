#pragma once

#include <erebus/binary.hxx>

#include <vector>


namespace Er
{

class EREBUS_EXPORT Variant final
{
public:
    using BoolVector = std::vector<Bool>;
    using Int32Vector = std::vector<int32_t>;
    using UInt32Vector = std::vector<uint32_t>;
    using Int64Vector = std::vector<int64_t>;
    using UInt64Vector = std::vector<uint64_t>;
    using DoubleVector = std::vector<double>;
    using StringVector = std::vector<std::string>;
    using BinaryVector = std::vector<Binary>;

    enum class Type: uint32_t
    {
        Empty,
        Bool,
        Int32,
        UInt32,
        Int64,
        UInt64,
        Double,
        String,
        Binary,
        Bools,
        Int32s,
        UInt32s,
        Int64s,
        UInt64s,
        Doubles,
        Strings,
        Binaries
    };

    ~Variant()
    {
        if (m_type >= Type::String)
            _free();
    }

    constexpr Variant() noexcept
        : m_u()
        , m_type(Type::Empty)
    {
    }

    Variant(nullptr_t) = delete;

    constexpr Variant(Bool v) noexcept
        : m_u()
        , m_type(Type::Bool)
    {
        m_u.v_bool = v;
    }

    constexpr Variant(int32_t v) noexcept
        : m_u()
        , m_type(Type::Int32)
    {
        m_u.v_int32 = v;
    }

    constexpr Variant(uint32_t v) noexcept
        : m_u()
        , m_type(Type::UInt32)
    {
        m_u.v_uint32 = v;
    }

    constexpr Variant(int64_t v) noexcept
        : m_u(v)
        , m_type(Type::Int64)
    {
    }

    constexpr Variant(uint64_t v) noexcept
        : m_u(v)
        , m_type(Type::UInt64)
    {
    }

    constexpr Variant(double v) noexcept
        : m_u(v)
        , m_type(Type::Double)
    {
    }

    Variant(const std::string& v) noexcept
        : m_u(new std::string(v))
        , m_type(Type::String)
    {
    }

    Variant(std::string&& v) noexcept
        : m_u(new std::string(std::move(v)))
        , m_type(Type::String)
    {
    }

    Variant(const Binary& v) noexcept
        : m_u(new Binary(v))
        , m_type(Type::Binary)
    {
    }

    Variant(Binary&& v) noexcept
        : m_u(new Binary(std::move(v)))
        , m_type(Type::Binary)
    {
    }

    Variant(const BoolVector& v) noexcept
        : m_u(new BoolVector(v))
        , m_type(Type::Bools)
    {
    }

    Variant(BoolVector&& v) noexcept
        : m_u(new BoolVector(std::move(v)))
        , m_type(Type::Bools)
    {
    }

    Variant(const Int32Vector& v) noexcept
        : m_u(new Int32Vector(v))
        , m_type(Type::Int32s)
    {
    }

    Variant(Int32Vector&& v) noexcept
        : m_u(new Int32Vector(std::move(v)))
        , m_type(Type::Int32s)
    {
    }

    Variant(const UInt32Vector& v) noexcept
        : m_u(new UInt32Vector(v))
        , m_type(Type::UInt32s)
    {
    }

    Variant(UInt32Vector&& v) noexcept
        : m_u(new UInt32Vector(std::move(v)))
        , m_type(Type::UInt32s)
    {
    }

    Variant(const Int64Vector& v) noexcept
        : m_u(new Int64Vector(v))
        , m_type(Type::Int64s)
    {
    }

    Variant(Int64Vector&& v) noexcept
        : m_u(new Int64Vector(std::move(v)))
        , m_type(Type::Int64s)
    {
    }

    Variant(const UInt64Vector& v) noexcept
        : m_u(new UInt64Vector(v))
        , m_type(Type::UInt64s)
    {
    }

    Variant(UInt64Vector&& v) noexcept
        : m_u(new UInt64Vector(std::move(v)))
        , m_type(Type::UInt64s)
    {
    }

    Variant(const DoubleVector& v) noexcept
        : m_u(new DoubleVector(v))
        , m_type(Type::Doubles)
    {
    }

    Variant(DoubleVector&& v) noexcept
        : m_u(new DoubleVector(std::move(v)))
        , m_type(Type::Doubles)
    {
    }

    Variant(const StringVector& v) noexcept
        : m_u(new StringVector(v))
        , m_type(Type::Strings)
    {
    }

    Variant(StringVector&& v) noexcept
        : m_u(new StringVector(std::move(v)))
        , m_type(Type::Strings)
    {
    }

    Variant(const BinaryVector& v) noexcept
        : m_u(new BinaryVector(v))
        , m_type(Type::Binaries)
    {
    }

    Variant(BinaryVector&& v) noexcept
        : m_u(new BinaryVector(std::move(v)))
        , m_type(Type::Binaries)
    {
    }

    constexpr void swap(Variant& other) noexcept
    {
        std::swap(m_type, other.m_type);
        std::swap(m_u._largest, other.m_u._largest);
    }

    constexpr Variant(const Variant& other)
        : m_u()
        , m_type(other.m_type)
    {
        if (other.m_type < Type::String)
            m_u._largest = other.m_u._largest;
        else
            _clone(other);
    }

    Variant& operator=(const Variant& other)
    {
        Variant tmp(other);
        swap(tmp);
        return *this;
    }

    constexpr Variant(Variant&& other) noexcept
        : Variant()
    {
        swap(other);
    }

    Variant& operator=(Variant&& other) noexcept
    {
        Variant tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    [[nodiscard]] constexpr Type type() const noexcept
    {
        return m_type;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return m_type == Type::Empty;
    }

    [[nodiscard]] constexpr const Bool& getBool() const noexcept
    {
        ErAssert(m_type == Type::Bool);
        return m_u.v_bool;
    }

    [[nodiscard]] constexpr const int32_t& getInt32() const noexcept
    {
        ErAssert(m_type == Type::Int32);
        return m_u.v_int32;
    }

    [[nodiscard]] constexpr const uint32_t& getUInt32() const noexcept
    {
        ErAssert(m_type == Type::UInt32);
        return m_u.v_uint32;
    }

    [[nodiscard]] constexpr const int64_t& getInt64() const noexcept
    {
        ErAssert(m_type == Type::Int64);
        return m_u.v_int64;
    }

    [[nodiscard]] constexpr const uint64_t& getUInt64() const noexcept
    {
        ErAssert(m_type == Type::UInt64);
        return m_u.v_uint64;
    }

    [[nodiscard]] constexpr const double& getDouble() const noexcept
    {
        ErAssert(m_type == Type::Double);
        return m_u.v_double;
    }

    [[nodiscard]] constexpr const std::string& getString() const noexcept
    {
        ErAssert(m_type == Type::String);
        ErAssert(m_u.v_string);
        return *m_u.v_string;
    }

    [[nodiscard]] constexpr const Binary& getBinary() const noexcept
    {
        ErAssert(m_type == Type::Binary);
        ErAssert(m_u.v_binary);
        return *m_u.v_binary;
    }

    [[nodiscard]] constexpr const BoolVector& getBools() const noexcept
    {
        ErAssert(m_type == Type::Bools);
        ErAssert(m_u.a_bool);
        return *m_u.a_bool;
    }

    [[nodiscard]] constexpr const Int32Vector& getInt32s() const noexcept
    {
        ErAssert(m_type == Type::Int32s);
        ErAssert(m_u.a_int32);
        return *m_u.a_int32;
    }

    [[nodiscard]] constexpr const UInt32Vector& getUInt32s() const noexcept
    {
        ErAssert(m_type == Type::UInt32s);
        ErAssert(m_u.a_uint32);
        return *m_u.a_uint32;
    }

    [[nodiscard]] constexpr const Int64Vector& getInt64s() const noexcept
    {
        ErAssert(m_type == Type::Int64s);
        ErAssert(m_u.a_int64);
        return *m_u.a_int64;
    }

    [[nodiscard]] constexpr const UInt64Vector& getUInt64s() const noexcept
    {
        ErAssert(m_type == Type::UInt64s);
        ErAssert(m_u.a_uint64);
        return *m_u.a_uint64;
    }

    [[nodiscard]] constexpr const DoubleVector& getDoubles() const noexcept
    {
        ErAssert(m_type == Type::Doubles);
        ErAssert(m_u.a_double);
        return *m_u.a_double;
    }

    [[nodiscard]] constexpr const StringVector& getStrings() const noexcept
    {
        ErAssert(m_type == Type::Strings);
        ErAssert(m_u.a_string);
        return *m_u.a_string;
    }

    [[nodiscard]] constexpr const BinaryVector& getBinaries() const noexcept
    {
        ErAssert(m_type == Type::Binaries);
        ErAssert(m_u.a_binary);
        return *m_u.a_binary;
    }

    [[nodiscard]] constexpr bool operator==(const Variant& other) const noexcept
    {
        if (m_type != other.m_type)
            return false;

        if (m_type == Type::Empty)
            return true;

        if (m_u._largest == other.m_u._largest)
            return true;

        if (m_type < Type::String)
            return false; // enough for scalar types

        return _eq(other);
    }

private:
    void _free() noexcept;
    void _freeString() noexcept;
    void _freeBinary() noexcept;
    void _freeBoolV() noexcept;
    void _freeInt32V() noexcept;
    void _freeUInt32V() noexcept;
    void _freeInt64V() noexcept;
    void _freeUInt64V() noexcept;
    void _freeDoubleV() noexcept;
    void _freeStringV() noexcept;
    void _freeBinaryV() noexcept;
    void _clone(const Variant& other);
    void _cloneString(const Variant& other);
    void _cloneBinary(const Variant& other);
    void _cloneBoolV(const Variant& other);
    void _cloneInt32V(const Variant& other);
    void _cloneUInt32V(const Variant& other);
    void _cloneInt64V(const Variant& other);
    void _cloneUInt64V(const Variant& other);
    void _cloneDoubleV(const Variant& other);
    void _cloneStringV(const Variant& other);
    void _cloneBinaryV(const Variant& other);
    bool _eq(const Variant& other) const noexcept;
    bool _eqString(const Variant& other) const noexcept;
    bool _eqBinary(const Variant& other) const noexcept;
    bool _eqBoolV(const Variant& other) const noexcept;
    bool _eqInt32V(const Variant& other) const noexcept;
    bool _eqUInt32V(const Variant& other) const noexcept;
    bool _eqInt64V(const Variant& other) const noexcept;
    bool _eqUInt64V(const Variant& other) const noexcept;
    bool _eqDoubleV(const Variant& other) const noexcept;
    bool _eqStringV(const Variant& other) const noexcept;
    bool _eqBinaryV(const Variant& other) const noexcept;

    union Storage
    {
        Bool v_bool;
        int32_t v_int32;
        uint32_t v_uint32;
        int64_t v_int64;
        uint64_t v_uint64;
        double v_double;
        void* _ptr;
        std::string* v_string;
        Binary* v_binary;
        BoolVector* a_bool;
        Int32Vector* a_int32;
        UInt32Vector* a_uint32;
        Int64Vector* a_int64;
        UInt64Vector* a_uint64;
        DoubleVector* a_double;
        StringVector* a_string;
        BinaryVector* a_binary;
        uint64_t _largest; // must be the largest type

        constexpr Storage()
            : _largest(0)
        {
        }

        constexpr Storage(double v)
            : v_double(v)
        {
        }

        constexpr Storage(int64_t v)
            : v_int64(v)
        {
        }

        constexpr Storage(uint64_t v)
            : v_uint64(v)
        {
        }

        constexpr Storage(void* v)
            : _ptr(v)
        {
        }
    } m_u;

    static_assert(sizeof(m_u) == sizeof(m_u._largest));

    Type m_type;
};

template <typename T>
const T& get(const Variant& v) noexcept;

template <>
[[nodiscard]] inline const Bool& get<>(const Variant& v) noexcept
{
    return v.getBool();
}

template <>
[[nodiscard]] inline const int32_t& get<>(const Variant& v) noexcept
{
    return v.getInt32();
}

template <>
[[nodiscard]] inline const uint32_t& get<>(const Variant& v) noexcept
{
    return v.getUInt32();
}

template <>
[[nodiscard]] inline const int64_t& get<>(const Variant& v) noexcept
{
    return v.getInt64();
}

template <>
[[nodiscard]] inline const uint64_t& get<>(const Variant& v) noexcept
{
    return v.getUInt64();
}

template <>
[[nodiscard]] inline const double& get<>(const Variant& v) noexcept
{
    return v.getDouble();
}

template <>
[[nodiscard]] inline const std::string& get<>(const Variant& v) noexcept
{
    return v.getString();
}

template <>
[[nodiscard]] inline const Binary& get<>(const Variant& v) noexcept
{
    return v.getBinary();
}

template <>
[[nodiscard]] inline const Variant::BoolVector& get<>(const Variant& v) noexcept
{
    return v.getBools();
}

template <>
[[nodiscard]] inline const Variant::Int32Vector& get<>(const Variant& v) noexcept
{
    return v.getInt32s();
}

template <>
[[nodiscard]] inline const Variant::UInt32Vector& get<>(const Variant& v) noexcept
{
    return v.getUInt32s();
}

template <>
[[nodiscard]] inline const Variant::Int64Vector& get<>(const Variant& v) noexcept
{
    return v.getInt64s();
}

template <>
[[nodiscard]] inline const Variant::UInt64Vector& get<>(const Variant& v) noexcept
{
    return v.getUInt64s();
}

template <>
[[nodiscard]] inline const Variant::DoubleVector& get<>(const Variant& v) noexcept
{
    return v.getDoubles();
}

template <>
[[nodiscard]] inline const Variant::StringVector& get<>(const Variant& v) noexcept
{
    return v.getStrings();
}

template <>
[[nodiscard]] inline const Variant::BinaryVector& get<>(const Variant& v) noexcept
{
    return v.getBinaries();
}


} // namespace Er {}