#pragma once

#include <erebus/rtl/luaxx/luaxx_state.hxx>

namespace Er
{

class ER_RTL_EXPORT LuaState final
    : public Er::Lua::State
{
public:
    ~LuaState();
    explicit LuaState(Er::Log::ILogger* log);

private:
    static int _print(lua_State* L);
    int print();
};


} // namespace Er {}
