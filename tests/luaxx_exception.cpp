#pragma once

#include "common.hpp"

#include <erebus/lua.hxx>

#include <exception>

static const std::string test_exceptions_script = R"(
function call_protected(function_name)
   ok, msg = pcall(_ENV[function_name])
   return ok, tostring(msg)
end

function raise(exception_message)
   error(exception_message)
end
)";

TEST(Lua, catch_exception_from_callback_within_lua) 
{
    Luaxx::State state(true);
    state.LoadFromString(test_exceptions_script);
    state["throw_logic_error"] =
        []() {throw std::logic_error("Message from C++.");};
    bool ok = true;
    std::string msg;
    Luaxx::tie(ok, msg) = state["call_protected"]("throw_logic_error");
    EXPECT_FALSE(ok);
    EXPECT_NE(msg.find("Message from C++."), std::string::npos);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, catch_unknwon_exception_from_callback_within_lua) 
{
    Luaxx::State state(true);
    state.LoadFromString(test_exceptions_script);
    state["throw_int"] =
        []() {throw 0;};
    bool ok = true;
    std::string msg;
    Luaxx::tie(ok, msg) = state["call_protected"]("throw_int");
    EXPECT_FALSE(ok);
    EXPECT_NE(msg.find("<Unknown exception>"), std::string::npos);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, call_exception_handler_for_exception_from_lua) 
{
    Luaxx::State state(true);
    state.LoadFromString(test_exceptions_script);
    int luaStatusCode = LUA_OK;
    std::string message;
    state.HandleExceptionsWith([&luaStatusCode, &message](int s, std::string msg, std::exception_ptr exception) 
    {
        luaStatusCode = s, message = std::move(msg);
    });
    state["raise"]("Message from Lua.");
    EXPECT_EQ(luaStatusCode, LUA_ERRRUN);
    EXPECT_NE(message.find("Message from Lua."), std::string::npos);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, call_exception_handler_for_exception_from_callback) 
{
    Luaxx::State state(true);
    int luaStatusCode = LUA_OK;
    std::string message;
    state.HandleExceptionsWith([&luaStatusCode, &message](int s, std::string msg, std::exception_ptr exception) 
    {
        luaStatusCode = s, message = std::move(msg);
    });
    state["throw_logic_error"] =
        []() {throw std::logic_error("Message from C++.");};
    state["throw_logic_error"]();
    EXPECT_EQ(luaStatusCode, LUA_ERRRUN);
    EXPECT_NE(message.find("Message from C++."), std::string::npos);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, call_exception_handler_while_using_sel_function) 
{
    Luaxx::State state(true);
    state.LoadFromString(test_exceptions_script);
    int luaStatusCode = LUA_OK;
    std::string message;
    state.HandleExceptionsWith([&luaStatusCode, &message](int s, std::string msg, std::exception_ptr exception) {
        luaStatusCode = s, message = std::move(msg);
    });
    Luaxx::function<void(std::string)> raiseFromLua = state["raise"];
    raiseFromLua("Message from Lua.");
    EXPECT_EQ(luaStatusCode, LUA_ERRRUN);
    EXPECT_NE(message.find("Message from Lua."), std::string::npos);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, rethrow_exception_for_exception_from_callback) 
{
    Luaxx::State state(true);
    state.HandleExceptionsWith([](int s, std::string msg, std::exception_ptr exception) 
    {
        if(exception) 
        {
            std::rethrow_exception(exception);
        }
    });
    state["throw_logic_error"] =
        []() {throw std::logic_error("Arbitrary message.");};
    
    bool expected = false;
    try 
    {
        state["throw_logic_error"]();
    } 
    catch(std::logic_error & e) 
    {
        EXPECT_NE(std::string(e.what()).find("Arbitrary message."), std::string::npos);
        expected = true;
    }
    EXPECT_TRUE(expected);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, rethrow_using_sel_function) 
{
    Luaxx::State state(true);
    state.HandleExceptionsWith([](int s, std::string msg, std::exception_ptr exception) 
    {
        if(exception) 
        {
            std::rethrow_exception(exception);
        }
    });
    state["throw_logic_error"] =
        []() {throw std::logic_error("Arbitrary message.");};
    Luaxx::function<void(void)> cause_exception = state["throw_logic_error"];
    bool expected = false;
    try 
    {
        cause_exception();
    } 
    catch(std::logic_error & e) 
    {
        expected = true;
        EXPECT_NE(std::string(e.what()).find("Arbitrary message."), std::string::npos);
    }
    EXPECT_TRUE(expected);
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, throw_on_exception_using_Load) 
{
    Luaxx::State state(true);
    state.HandleExceptionsWith([](int s, std::string msg, std::exception_ptr exception) 
    {
        throw std::logic_error(msg);
    });
    bool expected = false;
    try 
    {
        state.LoadFromFile("non_existing_file");
    } 
    catch (std::logic_error & e) 
    {
        expected = true;
        EXPECT_NE(std::string(e.what()).find("non_existing_file"), std::string::npos);
    }
    EXPECT_TRUE(expected);
    EXPECT_EQ(state.Size(), 0);
}
