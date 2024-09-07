#pragma once

#include <erebus/binary.hxx>

#include <vector>


namespace Er
{

class EREBUS_EXPORT Variant final
{
public:
    using BoolV = std::vector<bool>;
    using Int32V = std::vector<int32_t>;
    using UInt32V = std::vector<uint32_t>;
    using Int64V = std::vector<int64_t>;
    using UInt64V = std::vector<uint64_t>;
    using DoubleV = std::vector<double>;
    using StringV = std::vector<std::string>;
    using BinaryV = std::vector<Binary>;

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
        : m_type(Type::Empty)
    {
        m_u._largest = 0;
    }

    Variant(nullptr_t) = delete;

    constexpr Variant(bool v) noexcept
        : m_type(Type::Bool)
    {
        m_u.v_bool = v;
    }

    constexpr Variant(int32_t v) noexcept
        : m_type(Type::Int32)
    {
        m_u.v_int32 = v;
    }

    constexpr Variant(uint32_t v) noexcept
        : m_type(Type::UInt32)
    {
        m_u.v_uint32 = v;
    }

    constexpr Variant(int64_t v) noexcept
        : m_type(Type::Int64)
    {
        m_u.v_int64 = v;
    }

    constexpr Variant(uint64_t v) noexcept
        : m_type(Type::UInt64)
    {
        m_u.v_uint64 = v;
    }

    constexpr Variant(double v) noexcept
        : m_type(Type::Double)
    {
        m_u.v_double = v;
    }

    constexpr Variant(const std::string& v) noexcept
        : m_type(Type::String)
    {
        m_u.v_string = new std::string(v);
    }

    constexpr Variant(std::string&& v) noexcept
        : m_type(Type::String)
    {
        m_u.v_string = new std::string(std::move(v));
    }

    constexpr Variant(const Binary& v) noexcept
        : m_type(Type::Binary)
    {
        m_u.v_binary = new Binary(v);
    }

    constexpr Variant(Binary&& v) noexcept
        : m_type(Type::Binary)
    {
        m_u.v_binary = new Binary(std::move(v));
    }

    constexpr Variant(const BoolV& v) noexcept
        : m_type(Type::Bools)
    {
        m_u.a_bool = new BoolV(v);
    }

    constexpr Variant(BoolV&& v) noexcept
        : m_type(Type::Bools)
    {
        m_u.a_bool = new BoolV(std::move(v));
    }

    constexpr Variant(const Int32V& v) noexcept
        : m_type(Type::Int32s)
    {
        m_u.a_int32 = new Int32V(v);
    }

    constexpr Variant(Int32V&& v) noexcept
        : m_type(Type::Int32s)
    {
        m_u.a_int32 = new Int32V(std::move(v));
    }

    constexpr Variant(const UInt32V& v) noexcept
        : m_type(Type::UInt32s)
    {
        m_u.a_uint32 = new UInt32V(v);
    }

    constexpr Variant(UInt32V&& v) noexcept
        : m_type(Type::UInt32s)
    {
        m_u.a_uint32 = new UInt32V(std::move(v));
    }

    constexpr Variant(const Int64V& v) noexcept
        : m_type(Type::Int64s)
    {
        m_u.a_int64 = new Int64V(v);
    }

    constexpr Variant(Int64V&& v) noexcept
        : m_type(Type::Int64s)
    {
        m_u.a_int64 = new Int64V(std::move(v));
    }

    constexpr Variant(const UInt64V& v) noexcept
        : m_type(Type::UInt64s)
    {
        m_u.a_uint64 = new UInt64V(v);
    }

    constexpr Variant(UInt64V&& v) noexcept
        : m_type(Type::UInt64s)
    {
        m_u.a_uint64 = new UInt64V(std::move(v));
    }

    constexpr Variant(const DoubleV& v) noexcept
        : m_type(Type::Doubles)
    {
        m_u.a_double = new DoubleV(v);
    }

    constexpr Variant(DoubleV&& v) noexcept
        : m_type(Type::Doubles)
    {
        m_u.a_double = new DoubleV(std::move(v));
    }

    constexpr Variant(const StringV& v) noexcept
        : m_type(Type::Strings)
    {
        m_u.a_string = new StringV(v);
    }

    constexpr Variant(StringV&& v) noexcept
        : m_type(Type::Strings)
    {
        m_u.a_string = new StringV(std::move(v));
    }

    constexpr Variant(const BinaryV& v) noexcept
        : m_type(Type::Binaries)
    {
        m_u.a_binary = new BinaryV(v);
    }

    constexpr Variant(BinaryV&& v) noexcept
        : m_type(Type::Binaries)
    {
        m_u.a_binary = new BinaryV(std::move(v));
    }

    constexpr void swap(Variant& other) noexcept
    {
        std::swap(m_type, other.m_type);
        std::swap(m_u._largest, other.m_u._largest);
    }

    Variant(const Variant& other)
        : m_type(other.m_type)
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

    Variant(Variant&& other) noexcept
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

    [[nodiscard]] Type type() const noexcept
    {
        return m_type;
    }

    [[nodiscard]] bool empty() const noexcept
    {
        return m_type == Type::Empty;
    }

    [[nodiscard]] bool getBool() const noexcept
    {
        ErAssert(m_type == Type::Bool);
        return m_u.v_bool;
    }

    [[nodiscard]] int32_t getInt32() const noexcept
    {
        ErAssert(m_type == Type::Int32);
        return m_u.v_int32;
    }

    [[nodiscard]] uint32_t getUInt32() const noexcept
    {
        ErAssert(m_type == Type::UInt32);
        return m_u.v_uint32;
    }

    [[nodiscard]] int64_t getInt64() const noexcept
    {
        ErAssert(m_type == Type::Int64);
        return m_u.v_int64;
    }

    [[nodiscard]] uint64_t getUInt64() const noexcept
    {
        ErAssert(m_type == Type::UInt64);
        return m_u.v_uint64;
    }

    [[nodiscard]] double getDouble() const noexcept
    {
        ErAssert(m_type == Type::Double);
        return m_u.v_double;
    }

    [[nodiscard]] const std::string& getString() const noexcept
    {
        ErAssert(m_type == Type::String);
        ErAssert(m_u.v_string);
        return *m_u.v_string;
    }

    [[nodiscard]] const Binary& getBinary() const noexcept
    {
        ErAssert(m_type == Type::Binary);
        ErAssert(m_u.v_binary);
        return *m_u.v_binary;
    }

    [[nodiscard]] const BoolV& getBools() const noexcept
    {
        ErAssert(m_type == Type::Bools);
        ErAssert(m_u.a_bool);
        return *m_u.a_bool;
    }

    [[nodiscard]] const Int32V& getInt32s() const noexcept
    {
        ErAssert(m_type == Type::Int32s);
        ErAssert(m_u.a_int32);
        return *m_u.a_int32;
    }

    [[nodiscard]] const UInt32V& getUInt32s() const noexcept
    {
        ErAssert(m_type == Type::UInt32s);
        ErAssert(m_u.a_uint32);
        return *m_u.a_uint32;
    }

    [[nodiscard]] const Int64V& getInt64s() const noexcept
    {
        ErAssert(m_type == Type::Int64s);
        ErAssert(m_u.a_int64);
        return *m_u.a_int64;
    }

    [[nodiscard]] const UInt64V& getUInt64s() const noexcept
    {
        ErAssert(m_type == Type::UInt64s);
        ErAssert(m_u.a_uint64);
        return *m_u.a_uint64;
    }

    [[nodiscard]] const DoubleV& getDoubles() const noexcept
    {
        ErAssert(m_type == Type::Doubles);
        ErAssert(m_u.a_double);
        return *m_u.a_double;
    }

    [[nodiscard]] const StringV& getStrings() const noexcept
    {
        ErAssert(m_type == Type::Strings);
        ErAssert(m_u.a_string);
        return *m_u.a_string;
    }

    [[nodiscard]] const BinaryV& getBinaries() const noexcept
    {
        ErAssert(m_type == Type::Binaries);
        ErAssert(m_u.a_binary);
        return *m_u.a_binary;
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

    union
    {
        bool v_bool;
        int32_t v_int32;
        uint32_t v_uint32;
        int64_t v_int64;
        uint64_t v_uint64;
        double v_double;
        std::string* v_string;
        Binary* v_binary;
        BoolV* a_bool;
        Int32V* a_int32;
        UInt32V* a_uint32;
        Int64V* a_int64;
        UInt64V* a_uint64;
        DoubleV* a_double;
        StringV* a_string;
        BinaryV* a_binary;
        uint64_t _largest; // must be the largest type
    } m_u;

    static_assert(sizeof(m_u) == sizeof(m_u._largest));

    Type m_type;
};

} // namespace Er {}