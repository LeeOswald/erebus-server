#include <erebus/variant.hxx>

namespace Er
{

void Variant::_free() noexcept
{
    using FreeFn = void (Variant::*)() noexcept;

    static FreeFn s_freeFns[] =
    {
        &Variant::_freeString,
        &Variant::_freeBinary,
        &Variant::_freeBoolV,
        &Variant::_freeInt32V,
        &Variant::_freeUInt32V,
        &Variant::_freeInt64V,
        &Variant::_freeUInt64V,
        &Variant::_freeDoubleV,
        &Variant::_freeStringV,
        &Variant::_freeBinaryV
    };

    if (m_type < Type::String)
    {
        m_type = Type::Empty;
        return;
    }

    auto idx = static_cast<std::size_t>(m_type) - static_cast<std::size_t>(Type::String);
    ErAssert(idx < _countof(s_freeFns));
    std::invoke(s_freeFns[idx], *this);
}

void Variant::_freeString() noexcept
{
    ErAssert(m_type == Type::String);
    delete m_u.v_string;
    m_u.v_string = nullptr;
    m_type = Type::Empty;
}

void Variant::_freeBinary() noexcept
{
    ErAssert(m_type == Type::Binary);
    delete m_u.v_binary;
    m_u.v_binary = nullptr;
    m_type = Type::Empty;
}

void Variant::_freeBoolV() noexcept
{
    ErAssert(m_type == Type::Bools);
    delete m_u.a_bool;
    m_u.a_bool = nullptr;
    m_type = Type::Empty;
}

void Variant::_freeInt32V() noexcept
{
    ErAssert(m_type == Type::Int32s);
    delete m_u.a_int32;
    m_u.a_int32 = nullptr;
    m_type = Type::Empty;
}

void Variant::_freeUInt32V() noexcept
{
    ErAssert(m_type == Type::UInt32s);
    delete m_u.a_uint32;
    m_u.a_uint32 = nullptr;
    m_type = Type::Empty;
}

void Variant::_freeInt64V() noexcept
{
    ErAssert(m_type == Type::Int64s);
    delete m_u.a_int64;
    m_u.a_int64 = nullptr;
    m_type = Type::Empty;
}

void Variant::_freeUInt64V() noexcept
{
    ErAssert(m_type == Type::UInt64s);
    delete m_u.a_uint64;
    m_u.a_uint64 = nullptr;
    m_type = Type::Empty;
}

void Variant::_freeDoubleV() noexcept
{
    ErAssert(m_type == Type::Doubles);
    delete m_u.a_double;
    m_u.a_double = nullptr;
    m_type = Type::Empty;
}

void Variant::_freeStringV() noexcept
{
    ErAssert(m_type == Type::Strings);
    delete m_u.a_string;
    m_u.a_string = nullptr;
    m_type = Type::Empty;
}

void Variant::_freeBinaryV() noexcept
{
    ErAssert(m_type == Type::Binaries);
    delete m_u.a_binary;
    m_u.a_binary = nullptr;
    m_type = Type::Empty;
}

void Variant::_clone(const Variant& other)
{
    using CloneFn = void (Variant::*)(const Variant&);

    static CloneFn s_cloneFns[] =
    {
        &Variant::_cloneString,
        &Variant::_cloneBinary,
        &Variant::_cloneBoolV,
        &Variant::_cloneInt32V,
        &Variant::_cloneUInt32V,
        &Variant::_cloneInt64V,
        &Variant::_cloneUInt64V,
        &Variant::_cloneDoubleV,
        &Variant::_cloneStringV,
        &Variant::_cloneBinaryV
    };

    if (m_type < Type::String)
    {
        m_u._largest = other.m_u._largest;
        return;
    }

    auto idx = static_cast<std::size_t>(m_type) - static_cast<std::size_t>(Type::String);
    ErAssert(idx < _countof(s_cloneFns));
    std::invoke(s_cloneFns[idx], *this, other);
}

void Variant::_cloneString(const Variant& other)
{
    ErAssert(other.m_type == Type::String);
    ErAssert(other.m_u.v_string);
    m_u.v_string = new std::string(*other.m_u.v_string);
}

void Variant::_cloneBinary(const Variant& other)
{
    ErAssert(other.m_type == Type::Binary);
    ErAssert(other.m_u.v_binary);
    m_u.v_binary = new Binary(*other.m_u.v_binary);
}

void Variant::_cloneBoolV(const Variant& other)
{
    ErAssert(other.m_type == Type::Bools);
    ErAssert(other.m_u.a_bool);
    m_u.a_bool = new BoolV(*other.m_u.a_bool);
}

void Variant::_cloneInt32V(const Variant& other)
{
    ErAssert(other.m_type == Type::Int32s);
    ErAssert(other.m_u.a_int32);
    m_u.a_int32 = new Int32V(*other.m_u.a_int32);
}

void Variant::_cloneUInt32V(const Variant& other)
{
    ErAssert(other.m_type == Type::UInt32s);
    ErAssert(other.m_u.a_uint32);
    m_u.a_uint32 = new UInt32V(*other.m_u.a_uint32);
}

void Variant::_cloneInt64V(const Variant& other)
{
    ErAssert(other.m_type == Type::Int64s);
    ErAssert(other.m_u.a_int64);
    m_u.a_int64 = new Int64V(*other.m_u.a_int64);
}

void Variant::_cloneUInt64V(const Variant& other)
{
    ErAssert(other.m_type == Type::UInt64s);
    ErAssert(other.m_u.a_uint64);
    m_u.a_uint64 = new UInt64V(*other.m_u.a_uint64);
}

void Variant::_cloneDoubleV(const Variant& other)
{
    ErAssert(other.m_type == Type::Doubles);
    ErAssert(other.m_u.a_double);
    m_u.a_double = new DoubleV(*other.m_u.a_double);
}

void Variant::_cloneStringV(const Variant& other)
{
    ErAssert(other.m_type == Type::Strings);
    ErAssert(other.m_u.a_string);
    m_u.a_string = new StringV(*other.m_u.a_string);
}

void Variant::_cloneBinaryV(const Variant& other)
{
    ErAssert(other.m_type == Type::Binaries);
    ErAssert(other.m_u.a_binary);
    m_u.a_binary = new BinaryV(*other.m_u.a_binary);
}


} // namespace Er {}