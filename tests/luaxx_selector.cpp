#pragma once

#include "common.hpp"

#include <erebus/lua.hxx>

static const std::string test_script = R"(
function foo()
end

function add(a, b)
   return a + b
end

function sum_and_difference(a, b)
   return (a+b), (a-b)
end

function bar()
   return 4, true, "hi"
end

function execute()
   return cadd(5, 6);
end

function doozy(a)
   x, y = doozy_c(a, 2 * a)
   return x * y
end

mytable = {}
function mytable.foo()
   return 4
end

function embedded_nulls()
   return "\0h\0i"
end

my_global = 4

my_table = {}
my_table[3] = "hi"
my_table["key"] = 6.4

nested_table = {}
nested_table[2] = -3;
nested_table["foo"] = "bar";

my_table["nested"] = nested_table

global1 = 5
global2 = 5

function resumable()
   coroutine.yield(1)
   coroutine.yield(2)
   return 3
end

co = coroutine.create(resumable)

function resume_co()
   ran, value = coroutine.resume(co)
   return value
end

function set_global()
   global1 = 8
end

should_be_one = 0

function should_run_once()
   should_be_one = should_be_one + 1
end
)";

TEST(Lua, select_global) 
{
    sel::State state(true);
    state.LoadFromString(test_script);
    int answer = state["my_global"];
    EXPECT_EQ(answer, 4);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, select_field) 
{
    sel::State state(true);
    state.LoadFromString(test_script);
    lua_Number answer = state["my_table"]["key"];
    EXPECT_EQ(answer, lua_Number(6.4));
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, select_index) 
{
    sel::State state(true);
    state.LoadFromString(test_script);
    std::string answer = state["my_table"][3];
    EXPECT_STREQ(answer.c_str(), "hi");
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, select_nested_field) 
{
    sel::State state(true);
    state.LoadFromString(test_script);
    std::string answer = state["my_table"]["nested"]["foo"];
    EXPECT_STREQ(answer.c_str(), "bar");
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, select_nested_index) 
{
    sel::State state(true);
    state.LoadFromString(test_script);
    int answer = state["my_table"]["nested"][2];
    EXPECT_EQ(answer, -3);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, select_equality) 
{
    sel::State state(true);
    state.LoadFromString(test_script);
    EXPECT_EQ(state["my_table"]["nested"][2], -3);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, select_cast) 
{
    sel::State state(true);
    state.LoadFromString(test_script);
    EXPECT_EQ(int(state["global1"]), state["global2"]);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, set_global) 
{
    sel::State state(true);
    state.LoadFromString(test_script);
    auto lua_dummy_global = state["dummy_global"];
    lua_dummy_global = 32;
    EXPECT_EQ(state["dummy_global"], 32);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, set_field) 
{
    sel::State state(true);
    state.LoadFromString(test_script);
    state["my_table"]["dummy_key"] = "testing";
    std::string s = state["my_table"]["dummy_key"];
    EXPECT_STREQ(s.c_str(), "testing");
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, set_index) 
{
    sel::State state(true);
    state.LoadFromString(test_script);
    state["my_table"][10] = 3;
    EXPECT_EQ(state["my_table"][10], 3);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, set_nested_field) 
{
    sel::State state(true);
    state.LoadFromString(test_script);
    state["my_table"]["nested"]["asdf"] = true;
    EXPECT_TRUE((bool)state["my_table"]["nested"]["asdf"]);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, set_nested_index) 
{
    sel::State state(true);
    state.LoadFromString(test_script);
    state["my_table"]["nested"][1] = 2;
    EXPECT_EQ(state["my_table"]["nested"][1], 2);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, create_table_field) 
{
    sel::State state(true);
    state["new_table"]["test"] = 4;
    EXPECT_EQ(state["new_table"]["test"], 4);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, create_table_index) 
{
    sel::State state(true);
    state["new_table"][3] = 4;
    EXPECT_EQ(state["new_table"][3], 4);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, cache_selector_field_assignment) 
{
    sel::State state(true);
    sel::Selector s = state["new_table"][3];
    s = 4;
    EXPECT_EQ(state["new_table"][3], 4);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, cache_selector_field_access) 
{
    sel::State state(true);
    state["new_table"][3] = 4;
    sel::Selector s = state["new_table"][3];
    EXPECT_EQ(s, 4);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, cache_selector_function) 
{
    sel::State state(true);
    state.LoadFromString(test_script);
    sel::Selector s = state["set_global"];
    s();
    EXPECT_EQ(state["global1"], 8);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, function_should_run_once) 
{
    sel::State state(true);
    state.LoadFromString(test_script);
    auto should_run_once = state["should_run_once"];
    should_run_once();
    EXPECT_EQ(state["should_be_one"], 1);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, function_result_is_alive_ptr) 
{
    sel::State state(true);
    state["Obj"].SetClass<InstanceCounter>();
    state("function createObj() return Obj.new() end");
    int const instanceCountBeforeCreation = InstanceCounter::instances;

    sel::Pointer<InstanceCounter> pointer = state["createObj"]();
    state.ForceGC();

    EXPECT_EQ(InstanceCounter::instances, instanceCountBeforeCreation + 1);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, function_result_is_alive_ref) 
{
    sel::State state(true);
    state["Obj"].SetClass<InstanceCounter>();
    state("function createObj() return Obj.new() end");
    int const instanceCountBeforeCreation = InstanceCounter::instances;

    sel::Reference<InstanceCounter> reference = state["createObj"]();
    state.ForceGC();

    EXPECT_EQ(InstanceCounter::instances, instanceCountBeforeCreation + 1);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, get_and_set_Reference_keeps_identity) 
{
    sel::State state(true);
    state["Obj"].SetClass<InstanceCounter>();
    state("objA = Obj.new()");

    sel::Reference<InstanceCounter> objA_ref = state["objA"];
    state["objB"] = objA_ref;
    sel::Reference<InstanceCounter> objB_ref = state["objB"];

    state("function areVerySame() return objA == objB end");
    EXPECT_TRUE((bool)state["areVerySame"]());
    EXPECT_EQ(&objA_ref.get(), &objB_ref.get());
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, get_and_set_Pointer_keeps_identity) 
{
    sel::State state(true);
    state["Obj"].SetClass<InstanceCounter>();
    state("objA = Obj.new()");

    sel::Pointer<InstanceCounter> objA_ptr = state["objA"];
    state["objB"] = objA_ptr;
    sel::Pointer<InstanceCounter> objB_ptr = state["objB"];

    state("function areVerySame() return objA == objB end");
    EXPECT_TRUE((bool)state["areVerySame"]());
    EXPECT_EQ(objA_ptr, objB_ptr);
    EXPECT_EQ(state.Size(), 0);
}

struct SelectorBar 
{
};

struct SelectorFoo 
{
    int x;

    SelectorFoo(int num) 
        : x(num) 
    {}

    int getX() 
    {
        return x;
    }
};

TEST(Lua, selector_call_with_registered_class) 
{
    sel::State state(true);
    state["Foo"].SetClass<SelectorFoo, int>("get", &SelectorFoo::getX);
    state("function getXFromFoo(foo) return foo:get() end");
    SelectorFoo foo{4};
    EXPECT_EQ(state["getXFromFoo"](foo), 4);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, selector_call_with_registered_class_ptr) 
{
    sel::State state(true);
    state["Foo"].SetClass<SelectorFoo, int>("get", &SelectorFoo::getX);
    state("function getXFromFoo(foo) return foo:get() end");
    SelectorFoo foo{4};
    EXPECT_EQ(state["getXFromFoo"](&foo), 4);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, selector_call_with_wrong_type_ptr) 
{
    sel::State state(true);
    auto acceptFoo = [] (SelectorFoo *) {};
    state["Foo"].SetClass<SelectorFoo, int>();
    state["Bar"].SetClass<SelectorBar>();
    state["acceptFoo"] = acceptFoo;
    state("bar = Bar.new()");

    bool error_encounted = false;
    state.HandleExceptionsWith([&error_encounted](int, std::string, std::exception_ptr) 
    {
        error_encounted = true;
    });
    state("acceptFoo(bar)");

    EXPECT_TRUE(error_encounted);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, selector_call_with_wrong_type_ref) 
{
    sel::State state(true);
    auto acceptFoo = [] (SelectorFoo &) {};
    state["Foo"].SetClass<SelectorFoo, int>();
    state["Bar"].SetClass<SelectorBar>();
    state["acceptFoo"] = acceptFoo;
    state("bar = Bar.new()");

    bool error_encounted = false;
    state.HandleExceptionsWith([&error_encounted](int, std::string, std::exception_ptr) 
    {
        error_encounted = true;
    });
    state("acceptFoo(bar)");

    EXPECT_TRUE(error_encounted);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, selector_call_with_nullptr_ref) 
{
    sel::State state(true);
    auto acceptFoo = [] (SelectorFoo &) {};
    state["Foo"].SetClass<SelectorFoo, int>();
    state["acceptFoo"] = acceptFoo;

    bool error_encounted = false;
    state.HandleExceptionsWith([&error_encounted](int, std::string, std::exception_ptr) 
    {
        error_encounted = true;
    });
    state("acceptFoo(nil)");

    EXPECT_TRUE(error_encounted);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, selector_get_nullptr_ref) 
{
    sel::State state(true);
    state["Foo"].SetClass<SelectorFoo, int>();
    state("bar = nil");
    bool error_encounted = false;

    try
    {
        SelectorFoo & foo = state["bar"];
    } 
    catch(sel::TypeError &) 
    {
        error_encounted = true;
    }

    EXPECT_TRUE(error_encounted);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, selector_get_wrong_ref) 
{
    sel::State state(true);
    state["Foo"].SetClass<SelectorFoo, int>();
    state["Bar"].SetClass<SelectorBar>();
    state("bar = Bar.new()");
    bool error_encounted = false;

    try
    {
        SelectorFoo & foo = state["bar"];
    } 
    catch(sel::TypeError &) 
    {
        error_encounted = true;
    }

    EXPECT_TRUE(error_encounted);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, selector_get_wrong_ref_to_string) 
{
    sel::State state(true);
    state["Foo"].SetClass<SelectorFoo, int>();
    state("bar = \"Not a Foo\"");
    bool expected_message = false;

    try
    {
        SelectorFoo & foo = state["bar"];
    } 
    catch(sel::TypeError & e) 
    {
        expected_message = std::string(e.what()).find("got string") != std::string::npos;
    }

    EXPECT_TRUE(expected_message);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, selector_get_wrong_ref_to_table) 
{
    sel::State state(true);
    state["Foo"].SetClass<SelectorFoo, int>();
    state("bar = {}");
    bool expected_message = false;

    try
    {
        SelectorFoo & foo = state["bar"];
    } 
    catch(sel::TypeError & e) 
    {
        expected_message = std::string(e.what()).find("got table") != std::string::npos;
    }

    EXPECT_TRUE(expected_message);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, selector_get_wrong_ref_to_unregistered) 
{
    sel::State state(true);
    state["Foo"].SetClass<SelectorFoo, int>();
    state("foo = Foo.new(4)");
    bool expected_message = false;

    try
    {
        SelectorBar & bar = state["foo"];
    } 
    catch(sel::TypeError & e) 
    {
        expected_message = std::string(e.what()).find("unregistered type expected") != std::string::npos;
    }

    EXPECT_TRUE(expected_message);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, selector_get_wrong_ptr) 
{
    sel::State state(true);
    state["Foo"].SetClass<SelectorFoo, int>();
    state["Bar"].SetClass<SelectorBar>();
    state("bar = Bar.new()");
    SelectorFoo * foo = state["bar"];
    EXPECT_FALSE(!!foo);
    EXPECT_EQ(state.Size(), 0);
}
