#include <erebus/property.hxx>

#include <erebus/luaxx/luaxx_int64.hxx>
#include <erebus/luaxx/luaxx_property.hxx>
#include <erebus/luaxx/luaxx_state.hxx>

namespace Er::Lua
{

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

Er::Lua::Int64Wrapper getPropertyInt64(const Er::Property& prop)
{
    Er::Lua::Int64Wrapper w;
    auto p = std::get_if<int64_t>(&prop.value);
    w.value = p ? *p : 0;
    return w;
}

void setPropertyInt64(Er::Property& prop, const Er::Lua::Int64Wrapper& val)
{
    prop.value = val.value;
}

Er::Lua::UInt64Wrapper getPropertyUInt64(const Er::Property& prop)
{
    Er::Lua::UInt64Wrapper w;
    auto p = std::get_if<uint64_t>(&prop.value);
    w.value = p ? *p : 0;
    return w;
}

void setPropertyUInt64(Er::Property& prop, const Er::Lua::UInt64Wrapper& val)
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