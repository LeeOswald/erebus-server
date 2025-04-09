#pragma once

#include <iostream>
#include <memory>
#include <typeinfo>
#include <unordered_map>

#include <erebus/rtl/log.hxx>
#include <erebus/rtl/type_id.hxx>

extern "C" 
{
#include <lua.h>
#include <lauxlib.h>
}

namespace Er
{

template <typename T>
TypeIndex userType()
{
    auto id = typeId<T>().index();
    return id;
}

} // namespace Er {}

namespace Er::Lua 
{

namespace detail 
{

struct GetUserdataParameterFromLuaTypeError 
{
    std::string metatable_name;
    int index;
};

} // namespace detail {}

namespace MetatableRegistry 
{

using TypeID = TypeIndex;


ER_RTL_EXPORT void Create(lua_State* state);

ER_RTL_EXPORT void PushNewMetatable(lua_State* state, TypeID type, const std::string& name);

ER_RTL_EXPORT bool SetMetatable(lua_State* state, TypeID type);

ER_RTL_EXPORT bool IsRegisteredType(lua_State* state, TypeID type);

ER_RTL_EXPORT std::string GetTypeName(lua_State* state, TypeID type);

ER_RTL_EXPORT std::string GetTypeName(lua_State* state, int index);

ER_RTL_EXPORT bool IsType(lua_State* state, TypeID type, const int index);

ER_RTL_EXPORT void CheckType(lua_State* state, TypeID type, const int index);

} // namespace MetatableRegistry {}

} // namespace Er::Lua {}
