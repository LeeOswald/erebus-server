#pragma once

#include <iostream>
#include <memory>
#include <typeinfo>
#include <unordered_map>

extern "C" 
{
#include <lua.h>
#include <lauxlib.h>
}

namespace Er
{

EREBUS_EXPORT const void* registerUserType(const char* name);

template <typename T>
const void* userType()
{
    return registerUserType(typeid(T).name());
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

using TypeID = const void*;


EREBUS_EXPORT void Create(lua_State* state);

EREBUS_EXPORT void PushNewMetatable(lua_State* state, TypeID type, const std::string& name);

EREBUS_EXPORT bool SetMetatable(lua_State* state, TypeID type);

EREBUS_EXPORT bool IsRegisteredType(lua_State* state, TypeID type);

EREBUS_EXPORT std::string GetTypeName(lua_State* state, TypeID type);

EREBUS_EXPORT std::string GetTypeName(lua_State* state, int index);

EREBUS_EXPORT bool IsType(lua_State* state, TypeID type, const int index);

EREBUS_EXPORT void CheckType(lua_State* state, TypeID type, const int index);

} // namespace MetatableRegistry {}

} // namespace Er::Lua {}
