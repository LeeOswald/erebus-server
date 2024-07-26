#pragma once

#include "common.hpp"

#include <erebus/lua.hxx>

#include <iostream>

int take_fun_arg(sel::function<int(int, int)> fun, int a, int b) 
{
    return fun(a, b);
}

struct Mutator 
{
    Mutator() 
    {}

    Mutator(sel::function<void(int)> fun) 
    {
        fun(-4);
    }

    sel::function<void()> Foobar(bool which,
                                 sel::function<void()> foo,
                                 sel::function<void()> bar) 
    {
        return which ? foo : bar;
    }
};

static const std::string test_ref_script = R"(
function add(a, b)
   return a+b
end

function subtract(a, b)
   return a-b
end

function pass_add(x, y)
   return take_fun_arg(add, x, y)
end

function pass_sub(x, y)
   return take_fun_arg(subtract, x, y)
end

a = 4

function mutate_a(new_a)
   a = new_a
end

test = ""

function foo()
   test = "foo"
end

function bar()
   test = "bar"
end

function return_two()
   return 1, 2
end

)";

TEST(Lua, function_reference) 
{
    sel::State state(true);
    state["take_fun_arg"] = &take_fun_arg;
    state.LoadFromString(test_ref_script);
    bool check1 = state["pass_add"](3, 5) == 8;
    bool check2 = state["pass_sub"](4, 2) == 2;
    EXPECT_TRUE(check1);
    EXPECT_TRUE(check2);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, function_in_constructor) 
{
    sel::State state(true);
    state["Mutator"].SetClass<Mutator, sel::function<void(int)>>();
    state.LoadFromString(test_ref_script);
    bool check1 = state["a"] == 4;
    state("mutator = Mutator.new(mutate_a)");
    bool check2 = state["a"] == -4;
    EXPECT_TRUE(check1);
    EXPECT_TRUE(check2);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, pass_function_to_lua) 
{
    sel::State state(true);
    state["Mutator"].SetClass<Mutator>("foobar", &Mutator::Foobar);
    state.LoadFromString(test_ref_script);
    state("mutator = Mutator.new()");
    state("mutator:foobar(true, foo, bar)()");
    bool check1 = state["test"] == "foo";
    state("mutator:foobar(false, foo, bar)()");
    bool check2 = state["test"] == "bar";
    EXPECT_TRUE(check1);
    EXPECT_TRUE(check2);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, call_returned_lua_function) 
{
    sel::State state(true);
    state.LoadFromString(test_ref_script);
    sel::function<int(int, int)> lua_add = state["add"];
    EXPECT_EQ(lua_add(2, 4), 6);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, call_multivalue_lua_function) 
{
    sel::State state(true);
    state.LoadFromString(test_ref_script);
    sel::function<std::tuple<int, int>()> lua_add = state["return_two"];
    EXPECT_EQ(lua_add(), std::make_tuple(1, 2));
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, call_result_is_alive_ptr) 
{
    sel::State state(true);
    state["Obj"].SetClass<InstanceCounter>();
    state("function createObj() return Obj.new() end");
    sel::function<sel::Pointer<InstanceCounter>()> createObj = state["createObj"];
    int const instanceCountBeforeCreation = InstanceCounter::instances;

    sel::Pointer<InstanceCounter> pointer = createObj();
    state.ForceGC();

    EXPECT_EQ(InstanceCounter::instances, instanceCountBeforeCreation + 1);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, call_result_is_alive_ref) 
{
    sel::State state(true);
    state["Obj"].SetClass<InstanceCounter>();
    state("function createObj() return Obj.new() end");
    sel::function<sel::Reference<InstanceCounter>()> createObj = state["createObj"];
    int const instanceCountBeforeCreation = InstanceCounter::instances;

    sel::Reference<InstanceCounter> ref = createObj();
    state.ForceGC();

    EXPECT_EQ(InstanceCounter::instances, instanceCountBeforeCreation + 1);
    EXPECT_EQ(state.Size(), 0);
}

struct FunctionFoo 
{
    int x;

    FunctionFoo(int num) 
        : x(num) 
    {}
    
    int getX() 
    {
        return x;
    }
};

struct FunctionBar 
{
};

TEST(Lua, function_call_with_registered_class) 
{
    sel::State state(true);
    state["Foo"].SetClass<FunctionFoo, int>("get", &FunctionFoo::getX);
    state("function getX(foo) return foo:get() end");
    sel::function<int(FunctionFoo &)> getX = state["getX"];
    FunctionFoo foo{4};
    EXPECT_EQ(getX(foo), 4);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, function_call_with_registered_class_ptr) 
{
    sel::State state(true);
    state["Foo"].SetClass<FunctionFoo, int>("get", &FunctionFoo::getX);
    state("function getX(foo) return foo:get() end");
    sel::function<int(FunctionFoo *)> getX = state["getX"];
    FunctionFoo foo{4};
    EXPECT_EQ(getX(&foo), 4);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, function_call_with_registered_class_val) 
{
    sel::State state(true);
    state["Foo"].SetClass<FunctionFoo, int>("get", &FunctionFoo::getX);
    state("function store(foo) globalFoo = foo end");
    state("function getX() return globalFoo:get() end");

    sel::function<void(FunctionFoo)> store = state["store"];
    sel::function<int()> getX = state["getX"];
    store(FunctionFoo{4});

    EXPECT_EQ(getX(), 4);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, function_call_with_registered_class_val_lifetime) 
{
    sel::State state(true);
    state["Foo"].SetClass<InstanceCounter>();
    state("function store(foo) globalFoo = foo end");
    sel::function<void(InstanceCounter)> store = state["store"];

    int instanceCountBefore = InstanceCounter::instances;
    store(InstanceCounter{});

    EXPECT_EQ(InstanceCounter::instances, instanceCountBefore + 1);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, function_call_with_nullptr_ref) 
{
    sel::State state(true);
    state["Foo"].SetClass<FunctionFoo, int>();
    state("function makeNil() return nil end");
    sel::function<FunctionFoo &()> getFoo = state["makeNil"];
    bool error_encounted = false;

    try 
    {
        FunctionFoo & foo = getFoo();
    } 
    catch(sel::TypeError &) 
    {
        error_encounted = true;
    }

    EXPECT_TRUE(error_encounted);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, function_call_with_wrong_ref) 
{
    sel::State state(true);
    state["Foo"].SetClass<FunctionFoo, int>();
    state["Bar"].SetClass<FunctionBar>();
    state("function makeBar() return Bar.new() end");
    sel::function<FunctionFoo &()> getFoo = state["makeBar"];
    bool error_encounted = false;

    try 
    {
        FunctionFoo & foo = getFoo();
    } 
    catch(sel::TypeError &) 
    {
        error_encounted = true;
    }

    EXPECT_TRUE(error_encounted);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, function_call_with_wrong_ptr) 
{
    sel::State state(true);
    state["Foo"].SetClass<FunctionFoo, int>();
    state["Bar"].SetClass<FunctionBar>();
    state("function makeBar() return Bar.new() end");
    sel::function<FunctionFoo *()> getFoo = state["makeBar"];
    EXPECT_FALSE(!!getFoo());
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, function_get_registered_class_by_value) 
{
    sel::State state(true);
    state["Foo"].SetClass<FunctionFoo, int>();
    state("function getFoo() return Foo.new(4) end");
    sel::function<FunctionFoo()> getFoo = state["getFoo"];

    FunctionFoo foo = getFoo();

    EXPECT_EQ(foo.getX(), 4);
    EXPECT_EQ(state.Size(), 0);
}
