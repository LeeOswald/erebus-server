#include <erebus/exception.hxx>
#include <erebus/property.hxx>
#include <erebus/util/format.hxx>

#include <erebus/luaxx/luaxx_int64.hxx>
#include <erebus/luaxx/luaxx_property.hxx>
#include <erebus/luaxx/luaxx_state.hxx>

namespace Er::Lua
{

class EREBUS_EXPORT PropertyException
    : public LuaException
{
public:
    explicit PropertyException(Location&& location, const char* action, Er::PropId id, const char* expected, const char* actual)
        : LuaException(std::move(location), Er::Util::format("Failed to %s property %08x: expected a \'%s\', got \'%s\'", action, id, expected, actual))
    {}
};

namespace
{

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
    if (!p) [[unlikely]]
        throw PropertyException(ER_HERE(), "get", prop.id, "Bool", Er::propertyTypeToString(prop.type()));
    return *p;
}

void setPropertyBool(Er::Property& prop, bool val)
{
    if (prop.type() != PropertyType::Bool) [[unlikely]]
        throw PropertyException(ER_HERE(), "set", prop.id, "Bool", Er::propertyTypeToString(prop.type()));
    prop.value = val;
}

int32_t getPropertyInt32(const Er::Property& prop)
{
    auto p = std::get_if<int32_t>(&prop.value);
    if (!p) [[unlikely]]
        throw PropertyException(ER_HERE(), "get", prop.id, "Int32", Er::propertyTypeToString(prop.type()));
    return *p;
}

void setPropertyInt32(Er::Property& prop, int32_t val)
{
    if (prop.type() != PropertyType::Int32) [[unlikely]]
        throw PropertyException(ER_HERE(), "set", prop.id, "Int32", Er::propertyTypeToString(prop.type()));
    prop.value = val;
}

uint32_t getPropertyUInt32(const Er::Property& prop)
{
    auto p = std::get_if<uint32_t>(&prop.value);
    if (!p) [[unlikely]]
        throw PropertyException(ER_HERE(), "get", prop.id, "UInt32", Er::propertyTypeToString(prop.type()));
    return *p;
}

void setPropertyUInt32(Er::Property& prop, uint32_t val)
{
    if (prop.type() != PropertyType::UInt32) [[unlikely]]
        throw PropertyException(ER_HERE(), "set", prop.id, "UInt32", Er::propertyTypeToString(prop.type()));
    prop.value = val;
}

Er::Lua::Int64Wrapper getPropertyInt64(const Er::Property& prop)
{
    auto p = std::get_if<int64_t>(&prop.value);
    if (!p) [[unlikely]]
        throw PropertyException(ER_HERE(), "get", prop.id, "Int64", Er::propertyTypeToString(prop.type()));
    
    return Er::Lua::Int64Wrapper(*p);
}

void setPropertyInt64(Er::Property& prop, const Er::Lua::Int64Wrapper& val)
{
    if (prop.type() != PropertyType::Int64) [[unlikely]]
        throw PropertyException(ER_HERE(), "set", prop.id, "Int64", Er::propertyTypeToString(prop.type()));
    prop.value = val.value;
}

Er::Lua::UInt64Wrapper getPropertyUInt64(const Er::Property& prop)
{
    auto p = std::get_if<uint64_t>(&prop.value);
    if (!p) [[unlikely]]
        throw PropertyException(ER_HERE(), "get", prop.id, "UInt64", Er::propertyTypeToString(prop.type()));

    return Er::Lua::UInt64Wrapper(*p);
}

void setPropertyUInt64(Er::Property& prop, const Er::Lua::UInt64Wrapper& val)
{
    if (prop.type() != PropertyType::UInt64) [[unlikely]]
        throw PropertyException(ER_HERE(), "set", prop.id, "UInt64", Er::propertyTypeToString(prop.type()));
    prop.value = val.value;
}

double getPropertyDouble(const Er::Property& prop)
{
    auto p = std::get_if<double>(&prop.value);
    if (!p) [[unlikely]]
        throw PropertyException(ER_HERE(), "get", prop.id, "Double", Er::propertyTypeToString(prop.type()));
    return *p;
}

void setPropertyDouble(Er::Property& prop, double val)
{
    if (prop.type() != PropertyType::Double) [[unlikely]]
        throw PropertyException(ER_HERE(), "set", prop.id, "Double", Er::propertyTypeToString(prop.type()));
    prop.value = val;
}

std::string getPropertyString(const Er::Property& prop)
{
    auto p = std::get_if<std::string>(&prop.value);
    if (!p) [[unlikely]]
        throw PropertyException(ER_HERE(), "get", prop.id, "String", Er::propertyTypeToString(prop.type()));
    return *p;
}

void setPropertyString(Er::Property& prop, const std::string& val)
{
    if (prop.type() != PropertyType::String) [[unlikely]]
        throw PropertyException(ER_HERE(), "set", prop.id, "String", Er::propertyTypeToString(prop.type()));
    prop.value = val;
}

std::string getPropertyBytes(const Er::Property& prop)
{
    auto p = std::get_if<Er::Bytes>(&prop.value);
    if (!p) [[unlikely]]
        throw PropertyException(ER_HERE(), "get", prop.id, "Bytes", Er::propertyTypeToString(prop.type()));
    return p->bytes();
}

void setPropertyBytes(Er::Property& prop, const std::string& val)
{
    if (prop.type() != PropertyType::Bytes) [[unlikely]]
        throw PropertyException(ER_HERE(), "set", prop.id, "Bytes", Er::propertyTypeToString(prop.type()));
    prop.value = Er::Bytes(val);
}


} // namespace {}


EREBUS_EXPORT void registerPropertyTypes(State& state)
{
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
}

} // namespace Er::Lua {}