#include <erebus/protocol.hxx>

namespace Er
{

namespace Protocol
{

namespace
{


void assignPropertyEmpty(erebus::Property& out, const Property& in)
{
    out.set_type(static_cast<uint32_t>(Er::PropertyType::Empty));
    out.clear_value();
}

void assignPropertyBool(erebus::Property& out, const Property& in)
{
    out.set_type(static_cast<uint32_t>(Er::PropertyType::Bool));
    out.set_v_bool(get<bool>(in.value));
}

void assignPropertyInt32(erebus::Property& out, const Property& in)
{
    out.set_type(static_cast<uint32_t>(Er::PropertyType::Int32));
    out.set_v_int32(get<int32_t>(in.value));
}

void assignPropertyUInt32(erebus::Property& out, const Property& in)
{
    out.set_type(static_cast<uint32_t>(Er::PropertyType::UInt32));
    out.set_v_uint32(get<uint32_t>(in.value));
}

void assignPropertyInt64(erebus::Property& out, const Property& in)
{
    out.set_type(static_cast<uint32_t>(Er::PropertyType::Int64));
    out.set_v_int64(get<int64_t>(in.value));
}

void assignPropertyUInt64(erebus::Property& out, const Property& in)
{
    out.set_type(static_cast<uint32_t>(Er::PropertyType::UInt64));
    out.set_v_uint64(get<uint64_t>(in.value));
}

void assignPropertyDouble(erebus::Property& out, const Property& in)
{
    out.set_type(static_cast<uint32_t>(Er::PropertyType::Double));
    out.set_v_double(get<double>(in.value));
}

void assignPropertyString(erebus::Property& out, const Property& in)
{
    out.set_type(static_cast<uint32_t>(Er::PropertyType::String));
    out.set_v_string(get<std::string>(in.value));
}

void assignPropertyBinary(erebus::Property& out, const Property& in)
{
    out.set_type(static_cast<uint32_t>(Er::PropertyType::Binary));
    out.set_v_binary(get<Binary>(in.value).bytes());
}

void assignPropertyBools(erebus::Property& out, const Property& in)
{
    out.set_type(static_cast<uint32_t>(Er::PropertyType::Bools));
    auto& src = get<BoolVector>(in.value);
    auto dst = out.mutable_a_bool();
    for (auto v : src)
    {
        dst->add_a(v);
    }
}

void assignPropertyInt32s(erebus::Property& out, const Property& in)
{
    out.set_type(static_cast<uint32_t>(Er::PropertyType::Int32s));
    auto& src = get<Int32Vector>(in.value);
    auto dst = out.mutable_a_int32();
    for (auto v : src)
    {
        dst->add_a(v);
    }
}

void assignPropertyUInt32s(erebus::Property& out, const Property& in)
{
    out.set_type(static_cast<uint32_t>(Er::PropertyType::UInt32s));
    auto& src = get<UInt32Vector>(in.value);
    auto dst = out.mutable_a_uint32();
    for (auto v : src)
    {
        dst->add_a(v);
    }
}

void assignPropertyInt64s(erebus::Property& out, const Property& in)
{
    out.set_type(static_cast<uint32_t>(Er::PropertyType::Int64s));
    auto& src = get<Int64Vector>(in.value);
    auto dst = out.mutable_a_int64();
    for (auto v : src)
    {
        dst->add_a(v);
    }
}

void assignPropertyUInt64s(erebus::Property& out, const Property& in)
{
    out.set_type(static_cast<uint32_t>(Er::PropertyType::UInt64s));
    auto& src = get<UInt64Vector>(in.value);
    auto dst = out.mutable_a_uint64();
    for (auto v : src)
    {
        dst->add_a(v);
    }
}

void assignPropertyDoubles(erebus::Property& out, const Property& in)
{
    out.set_type(static_cast<uint32_t>(Er::PropertyType::Doubles));
    auto& src = get<DoubleVector>(in.value);
    auto dst = out.mutable_a_double();
    for (auto v : src)
    {
        dst->add_a(v);
    }
}

void assignPropertyStrings(erebus::Property& out, const Property& in)
{
    out.set_type(static_cast<uint32_t>(Er::PropertyType::Strings));
    auto& src = get<StringVector>(in.value);
    auto dst = out.mutable_a_string();
    for (auto& v : src)
    {
        dst->add_a(v);
    }
}

void assignPropertyBinaries(erebus::Property& out, const Property& in)
{
    out.set_type(static_cast<uint32_t>(Er::PropertyType::Binaries));
    auto& src = get<BinaryVector>(in.value);
    auto dst = out.mutable_a_binary();
    for (auto& v : src)
    {
        dst->add_a(v.bytes());
    }
}


Property getPropertyEmpty(const erebus::Property& in)
{
    return Property();
}

Property getPropertyBool(const erebus::Property& in)
{
    return Property(PropId(in.id()), in.v_bool());
}

Property getPropertyInt32(const erebus::Property& in)
{
    return Property(PropId(in.id()), in.v_int32());
}

Property getPropertyUInt32(const erebus::Property& in)
{
    return Property(PropId(in.id()), in.v_uint32());
}

Property getPropertyInt64(const erebus::Property& in)
{
    return Property(PropId(in.id()), in.v_int64());
}

Property getPropertyUInt64(const erebus::Property& in)
{
    return Property(PropId(in.id()), in.v_uint64());
}

Property getPropertyDouble(const erebus::Property& in)
{
    return Property(PropId(in.id()), in.v_double());
}

Property getPropertyString(const erebus::Property& in)
{
    return Property(PropId(in.id()), in.v_string());
}

Property getPropertyBinary(const erebus::Property& in)
{
    return Property(PropId(in.id()), Binary(in.v_binary()));
}

Property getPropertyBools(const erebus::Property& in)
{
    BoolVector v;
    auto& a = in.a_bool();
    auto size = a.a_size();
    if (size)
    {
        v.reserve(size);
        for (int i = 0; i < size; ++i)
        {
            v.push_back(a.a(i));
        }
    }

    return Property(PropId(in.id()), std::move(v));
}

Property getPropertyInt32s(const erebus::Property& in)
{
    Int32Vector v;
    auto& a = in.a_int32();
    auto size = a.a_size();
    if (size)
    {
        v.reserve(size);
        for (int i = 0; i < size; ++i)
        {
            v.push_back(a.a(i));
        }
    }

    return Property(PropId(in.id()), std::move(v));
}

Property getPropertyUInt32s(const erebus::Property& in)
{
    UInt32Vector v;
    auto& a = in.a_uint32();
    auto size = a.a_size();
    if (size)
    {
        v.reserve(size);
        for (int i = 0; i < size; ++i)
        {
            v.push_back(a.a(i));
        }
    }

    return Property(PropId(in.id()), std::move(v));
}

Property getPropertyInt64s(const erebus::Property& in)
{
    Int64Vector v;
    auto& a = in.a_int64();
    auto size = a.a_size();
    if (size)
    {
        v.reserve(size);
        for (int i = 0; i < size; ++i)
        {
            v.push_back(a.a(i));
        }
    }

    return Property(PropId(in.id()), std::move(v));
}

Property getPropertyUInt64s(const erebus::Property& in)
{
    UInt64Vector v;
    auto& a = in.a_uint64();
    auto size = a.a_size();
    if (size)
    {
        v.reserve(size);
        for (int i = 0; i < size; ++i)
        {
            v.push_back(a.a(i));
        }
    }

    return Property(PropId(in.id()), std::move(v));
}

Property getPropertyDoubles(const erebus::Property& in)
{
    DoubleVector v;
    auto& a = in.a_double();
    auto size = a.a_size();
    if (size)
    {
        v.reserve(size);
        for (int i = 0; i < size; ++i)
        {
            v.push_back(a.a(i));
        }
    }

    return Property(PropId(in.id()), std::move(v));
}

Property getPropertyStrings(const erebus::Property& in)
{
    StringVector v;
    auto& a = in.a_string();
    auto size = a.a_size();
    if (size)
    {
        v.reserve(size);
        for (int i = 0; i < size; ++i)
        {
            v.push_back(a.a(i));
        }
    }

    return Property(PropId(in.id()), std::move(v));
}

Property getPropertyBinaries(const erebus::Property& in)
{
    BinaryVector v;
    auto& a = in.a_binary();
    auto size = a.a_size();
    if (size)
    {
        v.reserve(size);
        for (int i = 0; i < size; ++i)
        {
            v.push_back(Binary(a.a(i)));
        }
    }

    return Property(PropId(in.id()), std::move(v));
}

} // namespace {}


void assignProperty(erebus::Property& out, const Property& source)
{
    using AssignPropertyFn = void(*)(erebus::Property& out, const Property& in);
    
    static AssignPropertyFn s_assignPropertyFns[] =
    {
        &assignPropertyEmpty,
        &assignPropertyBool,
        &assignPropertyInt32,
        &assignPropertyUInt32,
        &assignPropertyInt64,
        &assignPropertyUInt64,
        &assignPropertyDouble,
        &assignPropertyString,
        &assignPropertyBinary,
        &assignPropertyBools,
        &assignPropertyInt32s,
        &assignPropertyUInt32s,
        &assignPropertyInt64s,
        &assignPropertyUInt64s,
        &assignPropertyDoubles,
        &assignPropertyStrings,
        &assignPropertyBinaries
    };

    out.set_id(source.id);

    auto idx = static_cast<std::size_t>(source.type());
    ErAssert(idx < _countof(s_assignPropertyFns));
    std::invoke(s_assignPropertyFns[idx], out, source);
}

Property getProperty(const erebus::Property& source)
{
    using GetPropertyFn = Property(*)(const erebus::Property& in);

    static GetPropertyFn s_getPropertyFns[] =
    {
        &getPropertyEmpty,
        &getPropertyBool,
        &getPropertyInt32,
        &getPropertyUInt32,
        &getPropertyInt64,
        &getPropertyUInt64,
        &getPropertyDouble,
        &getPropertyString,
        &getPropertyBinary,
        &getPropertyBools,
        &getPropertyInt32s,
        &getPropertyUInt32s,
        &getPropertyInt64s,
        &getPropertyUInt64s,
        &getPropertyDoubles,
        &getPropertyStrings,
        &getPropertyBinaries
    };
    
    auto idx = static_cast<std::size_t>(source.type());
    if (idx >= _countof(s_getPropertyFns))
        ErThrow(Er::Util::format("Unsupported property type %d", idx));

    return std::invoke(s_getPropertyFns[idx], source);
}


} // namespace Protocol {}

} // namespace Er {}