#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/luaxx.hxx>
#include <erebus/rtl/luaxx/luaxx_int64.hxx>


#include <sstream>

namespace Er
{

LuaState::~LuaState()
{
}
    
LuaState::LuaState(Er::Log::ILogger* log)
    : Er::Lua::State(log, true)
{
    // craft the global Lua stuff
    static const luaL_Reg globalFunctions[] = 
    {
        { "print", &LuaState::_print },
        { nullptr, nullptr }
    };

    lua_pushlightuserdata(m_l, this);
    lua_setglobal(m_l, "__this");

    lua_getglobal(m_l, "_G");
#if LUA_VERSION_NUM >= 502
    luaL_setfuncs(m_l, globalFunctions, 0);
#else
    luaL_register(m_l, nullptr, globalFunctions);
#endif
    lua_pop(m_l, 1);

    Er::Lua::registerInt64(*this);
    Er::Lua::registerUInt64(*this);
}

int LuaState::_print(lua_State* L)
{
    auto type = lua_getglobal(L, "__this");
    if (type != LUA_TLIGHTUSERDATA)
    {
        lua_pop(L, 1);
        return 0;
    }
    auto _this = static_cast<LuaState*>(lua_touserdata(L, -1));
    lua_pop(L, 1);

    return _this ? _this->print() : 0;
}

int LuaState::print()
{
    std::ostringstream ss;

    int n = lua_gettop(m_l);  // number of arguments
    int i;
    for (i = 1; i <= n; i++)
    {
        size_t l;
        const char* s = luaL_tolstring(m_l, i, &l);  // convert it to string 
        if (i > 1)  // not the first arg
            ss << " ";

        ss << s;
        
        lua_pop(m_l, 1);
    }

    m_log->write(Er::Log::Record::make(
        "lua",
        Er::Log::Level::Info,
        Er::Time::now(),
        Er::System::CurrentThread::id(),
        ss.str()
    ));

    return 0;
}


} // namespace Er {}