#include <erebus/exception.hxx>
#include <erebus/luaxx.hxx>


namespace Er
{

namespace
{

namespace PropertyAdapter
{

template <typename T, typename = void>
struct IntegerWrapper;

template <typename T> 
struct IntegerWrapper<T, std::enable_if_t<std::is_integral<T>::value && (sizeof(T) == 8), void>>
{
    using HalfType = std::conditional_t<std::is_signed_v<T>, int32_t, uint32_t>;
    T value;

    IntegerWrapper(HalfType hi = HalfType(), uint32_t lo = uint32_t()) : value((T(hi) << 32) | lo) {}
    void add(const IntegerWrapper& v) { value += v.value; }
    void sub(const IntegerWrapper& v) { value -= v.value; }
    void mul(const IntegerWrapper& v) { value *= v.value; }
    void div(const IntegerWrapper& v) { value /= v.value; }
    void mod(const IntegerWrapper& v) { value %= v.value; }
    void bit_and(const IntegerWrapper& v) { value &= v.value; }
    void bit_or(const IntegerWrapper& v) { value |= v.value; }
    void bit_xor(const IntegerWrapper& v) { value ^= v.value; }
    void bit_not() { value = ~value; }
};

using Int64Wrapper = IntegerWrapper<int64_t>;
using UInt64Wrapper = IntegerWrapper<uint64_t>;

void registerInt64(Er::LuaState& state)
{
    state["Er"]["Int64"].SetClass<Int64Wrapper, int32_t, uint32_t>(
        "add", &Int64Wrapper::add,
        "sub", &Int64Wrapper::sub,
        "mul", &Int64Wrapper::mul,
        "div", &Int64Wrapper::div,
        "mod", &Int64Wrapper::mod,
        "bit_and", &Int64Wrapper::bit_and,
        "bit_or", &Int64Wrapper::bit_or,
        "bit_xor", &Int64Wrapper::bit_xor,
        "bit_not", &Int64Wrapper::bit_not
    );
}

void registerUInt64(Er::LuaState& state)
{
    state["Er"]["UInt64"].SetClass<UInt64Wrapper, uint32_t, uint32_t>(
        "add", &UInt64Wrapper::add,
        "sub", &UInt64Wrapper::sub,
        "mul", &UInt64Wrapper::mul,
        "div", &UInt64Wrapper::div,
        "mod", &UInt64Wrapper::mod,
        "bit_and", &UInt64Wrapper::bit_and,
        "bit_or", &UInt64Wrapper::bit_or,
        "bit_xor", &UInt64Wrapper::bit_xor,
        "bit_not", &UInt64Wrapper::bit_not
    );
}

void registerPropertyTypes(Er::LuaState& state)
{
    Er::Lua::Selector s = state["Er"]["PropertyType"];
    s["Invalid"] = static_cast<uint32_t>(Er::PropertyType::Invalid);
    s["Empty"] = static_cast<uint32_t>(Er::PropertyType::Empty);
    s["Bool"] = static_cast<uint32_t>(Er::PropertyType::Bool);
    s["Int32"] = static_cast<uint32_t>(Er::PropertyType::Int32);
    s["UInt32"] = static_cast<uint32_t>(Er::PropertyType::UInt32);
    s["Int64"] = static_cast<uint32_t>(Er::PropertyType::Int64);
    s["UInt64"] = static_cast<uint32_t>(Er::PropertyType::UInt64);
    s["Double"] = static_cast<uint32_t>(Er::PropertyType::Double);
    s["String"] = static_cast<uint32_t>(Er::PropertyType::String);
    s["Bytes"] = static_cast<uint32_t>(Er::PropertyType::Bytes);
}

uint32_t getPropertyId(const Er::Property& prop)
{
    return prop.id;
}

uint32_t getPropertyType(const Er::Property& prop)
{
    return static_cast<uint32_t>(prop.type());
}

bool getPropertyBool(const Er::Property& prop)
{
    auto p = std::get_if<bool>(&prop.value);
    return p ? *p : false;
}

void setPropertyBool(Er::Property& prop, bool val)
{
    prop.value = val;
}

int32_t getPropertyInt32(const Er::Property& prop)
{
    auto p = std::get_if<int32_t>(&prop.value);
    return p ? *p : 0;
}

void setPropertyInt32(Er::Property& prop, int32_t val)
{
    prop.value = val;
}

uint32_t getPropertyUInt32(const Er::Property& prop)
{
    auto p = std::get_if<uint32_t>(&prop.value);
    return p ? *p : 0;
}

void setPropertyUInt32(Er::Property& prop, uint32_t val)
{
    prop.value = val;
}

Int64Wrapper getPropertyInt64(const Er::Property& prop)
{
    Int64Wrapper w;
    auto p = std::get_if<int64_t>(&prop.value);
    w.value = p ? *p : 0;
    return w;
}

void setPropertyInt64(Er::Property& prop, const Int64Wrapper& val)
{
    prop.value = val.value;
}

UInt64Wrapper getPropertyUInt64(const Er::Property& prop)
{
    UInt64Wrapper w;
    auto p = std::get_if<uint64_t>(&prop.value);
    w.value = p ? *p : 0;
    return w;
}

void setPropertyUInt64(Er::Property& prop, const UInt64Wrapper& val)
{
    prop.value = val.value;
}

double getPropertyDouble(const Er::Property& prop)
{
    auto p = std::get_if<double>(&prop.value);
    return p ? *p : 0;
}

void setPropertyDouble(Er::Property& prop, double val)
{
    prop.value = val;
}

std::string getPropertyString(const Er::Property& prop)
{
    auto p = std::get_if<std::string>(&prop.value);
    return p ? *p : std::string();
}

void setPropertyString(Er::Property& prop, const std::string& val)
{
    prop.value = val;
}

std::string getPropertyBytes(const Er::Property& prop)
{
    auto p = std::get_if<Er::Bytes>(&prop.value);
    return p ? p->bytes() : std::string();
}

void setPropertyBytes(Er::Property& prop, const std::string& val)
{
    prop.value = Er::Bytes(val);
}

void registerPropertyMethods(Er::LuaState& state)
{
    Er::Lua::Selector s = state["Er"]["Property"];
    s["getId"] = &getPropertyId;
    s["getType"] = &getPropertyType;
    s["getBool"] = &getPropertyBool;
    s["setBool"] = &setPropertyBool;
    s["getInt32"] = &getPropertyInt32;
    s["setInt32"] = &setPropertyInt32;
    s["getUInt32"] = &getPropertyUInt32;
    s["setUInt32"] = &setPropertyUInt32;
    s["getInt64"] = &getPropertyInt64;
    s["setInt64"] = &setPropertyInt64;
    s["getUInt64"] = &getPropertyUInt64;
    s["setUInt64"] = &setPropertyUInt64;
    s["getDouble"] = &getPropertyDouble;
    s["setDouble"] = &setPropertyDouble;
    s["getString"] = &getPropertyString;
    s["setString"] = &setPropertyString;
    s["getBytes"] = &getPropertyBytes;
    s["setBytes"] = &setPropertyBytes;
}

} // namespace PropertyAdapter {}

} // namespace {}

LuaState::~LuaState()
{
}
    
LuaState::LuaState(Er::Log::ILog* log)
    : Er::Lua::State(log, true)
{
    // craft the global Lua stuff
    PropertyAdapter::registerInt64(*this);
    PropertyAdapter::registerUInt64(*this);
    PropertyAdapter::registerPropertyTypes(*this);
    PropertyAdapter::registerPropertyMethods(*this);
}


} // namespace Er {}