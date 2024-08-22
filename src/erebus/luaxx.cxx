#include <erebus/exception.hxx>
#include <erebus/luaxx.hxx>


namespace Er
{

namespace
{

namespace PropertyAdapter
{

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

int32_t getPropertyInt32(const Er::Property& prop)
{
    auto p = std::get_if<int32_t>(&prop.value);
    return p ? *p : 0;
}

uint32_t getPropertyUInt32(const Er::Property& prop)
{
    auto p = std::get_if<uint32_t>(&prop.value);
    return p ? *p : 0;
}

int64_t getPropertyInt64(const Er::Property& prop)
{
    auto p = std::get_if<int64_t>(&prop.value);
    return p ? *p : 0;
}

uint64_t getPropertyUInt64(const Er::Property& prop)
{
    auto p = std::get_if<uint64_t>(&prop.value);
    return p ? *p : 0;
}

double getPropertyDouble(const Er::Property& prop)
{
    auto p = std::get_if<double>(&prop.value);
    return p ? *p : 0;
}

std::string getPropertyString(const Er::Property& prop)
{
    auto p = std::get_if<std::string>(&prop.value);
    return p ? *p : std::string();
}

std::string getPropertyBytes(const Er::Property& prop)
{
    auto p = std::get_if<Er::Bytes>(&prop.value);
    return p ? p->bytes() : std::string();
}

void registerPropertyMethods(Er::LuaState& state)
{
    Er::Lua::Selector s = state["Er"]["Property"];
    s["getId"] = &getPropertyId;
    s["getType"] = &getPropertyType;
    s["getBool"] = &getPropertyBool;
    s["getInt32"] = &getPropertyInt32;
    s["getUInt32"] = &getPropertyUInt32;
    s["getInt64"] = &getPropertyInt64;
    s["getUInt64"] = &getPropertyUInt64;
    s["getDouble"] = &getPropertyDouble;
    s["getString"] = &getPropertyString;
    s["getBytes"] = &getPropertyBytes;
}

} // namespace PropertyAdapter {}

} // namespace {}

LuaState::~LuaState()
{
    if (m_l)
    {
        lua_gc(m_l, LUA_GCCOLLECT, 0);
        lua_close(m_l);
    }
}
    
LuaState::LuaState(Er::Log::ILog* log)
    : m_log(log)
    , m_l(luaL_newstate())
{
    if (!m_l)
        throw Er::Exception(ER_HERE(), "Failed to allocate a Lua state");

    luaL_openlibs(m_l);

    m_registry.reset(new Er::Lua::Registry(m_l));
    m_exceptionHandler.reset(new Er::Lua::ExceptionHandler(
        [this](int luaStatusCode, std::string msg, std::exception_ptr exception)
        {
            exceptionHandler(luaStatusCode, msg, exception);
        }
    ));

    // craft the global Lua stuff
    PropertyAdapter::registerPropertyTypes(*this);
    PropertyAdapter::registerPropertyMethods(*this);
}

void LuaState::exceptionHandler(int luaStatusCode, std::string msg, std::exception_ptr exception)
{
    m_log->writef(Log::Level::Error, "[Lua] %s", msg.c_str());
}

bool LuaState::loadFromString(std::string_view str, const char* name)
{
    Er::Lua::ResetStackOnScopeExit savedStack(m_l);
    int status = luaL_loadbuffer(m_l, str.data(), str.length(), name);
#if LUA_VERSION_NUM >= 502
    auto const lua_ok = LUA_OK;
#else
    auto const lua_ok = 0;
#endif
    if (status != lua_ok)
    {
        if (status == LUA_ERRSYNTAX)
        {
            const char* msg = lua_tostring(m_l, -1);
            m_exceptionHandler->Handle(status, msg ? msg : "Failed to load script");
        }

        return false;
    }

    status = lua_pcall(m_l, 0, LUA_MULTRET, 0);
    if (status == lua_ok)
    {
        return true;
    }

    const char* msg = lua_tostring(m_l, -1);
    m_exceptionHandler->Handle(status, msg ? msg : "Failed to parse script");

    return false;
}


} // namespace Er {}