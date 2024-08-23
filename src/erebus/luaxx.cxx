#include <erebus/exception.hxx>
#include <erebus/luaxx.hxx>
#include <erebus/luaxx/luaxx_int64.hxx>
#include <erebus/luaxx/luaxx_property.hxx>

namespace Er
{

LuaState::~LuaState()
{
}
    
LuaState::LuaState(Er::Log::ILog* log)
    : Er::Lua::State(log, true)
{
    // craft the global Lua stuff
    Er::Lua::registerInt64(*this);
    Er::Lua::registerUInt64(*this);
    Er::Lua::registerPropertyTypes(*this);
}


} // namespace Er {}