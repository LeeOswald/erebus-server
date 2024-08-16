#pragma once

#include "common.hpp"

#include <erebus/lua.hxx>

struct Qux 
{
    int baz() { return 4; }
    int qux = 3;
};

static Qux qux;

Qux* GetQuxPtr() { return &qux; }
Qux& GetQuxRef() { return qux; }

static const std::string test_metatable_script = R"(
function call_method()
   instance = get_instance()
   return instance:baz()
end

function access_member()
   instance = get_instance()
   return instance:qux()
end
)";

TEST(Lua, metatable_registry_ptr) 
{
    Er::Lua::State state(true);
    state["get_instance"] = &GetQuxPtr;
    state["Qux"].SetClass<Qux>("baz", &Qux::baz);
    state.LoadFromString(test_metatable_script);
    EXPECT_EQ(state["call_method"](), 4);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, metatable_registry_ref) 
{
    Er::Lua::State state(true);
    state["get_instance"] = &GetQuxRef;
    state["Qux"].SetClass<Qux>("baz", &Qux::baz);
    state.LoadFromString(test_metatable_script);
    EXPECT_EQ(state["call_method"](), 4);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, metatable_ptr_member) 
{
    Er::Lua::State state(true);
    state["get_instance"] = &GetQuxPtr;
    state["Qux"].SetClass<Qux>("baz", &Qux::baz, "qux", &Qux::qux);
    state.LoadFromString(test_metatable_script);
    EXPECT_EQ(state["access_member"](), 3);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, metatable_ref_member) 
{
    Er::Lua::State state(true);
    state["get_instance"] = &GetQuxRef;
    state["Qux"].SetClass<Qux>("baz", &Qux::baz, "qux", &Qux::qux);
    state.LoadFromString(test_metatable_script);
    EXPECT_EQ(state["access_member"](), 3);
    EXPECT_EQ(state.Size(), 0);
}
