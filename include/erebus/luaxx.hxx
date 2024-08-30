#pragma once

#include <erebus/luaxx/luaxx_state.hxx>

namespace Er
{

class EREBUS_EXPORT LuaState final
    : public Er::Lua::State
{
public:
    ~LuaState();
    explicit LuaState(Er::Log::ILog* log);

private:
    static int _print(lua_State* L);
    int print();
};


EREBUS_EXPORT void initializeLua(Er::Log::ILog* log);
EREBUS_EXPORT void finalizeLua();

} // namespace Er {}
