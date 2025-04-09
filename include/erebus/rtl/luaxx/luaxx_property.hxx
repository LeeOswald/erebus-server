#pragma once

#include <erebus/rtl/platform.hxx>


namespace Er::Lua
{

class State;

ER_RTL_EXPORT void registerPropertyTypes(State& state);


} // namespace Er::Lua {}