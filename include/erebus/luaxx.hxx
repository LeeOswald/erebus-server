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
};


} // namespace Er {}
