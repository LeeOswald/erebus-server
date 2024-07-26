#pragma once

#include "common.hpp"

#include <erebus/lua.hxx>

#include <string>
#include <sstream>


class CapturedStdout 
{
public:
    CapturedStdout() 
    {
        _old = std::cout.rdbuf(_out.rdbuf());
    }

    ~CapturedStdout() 
    {
        std::cout.rdbuf(_old);
    }

    std::string Content() const 
    {
        return _out.str();
    }

private:
    std::stringstream _out;
    std::streambuf *_old;
};


TEST(Lua, load_error) 
{
    sel::State state(true);

    CapturedStdout capture;
    const char* expected = "cannot open";
    EXPECT_FALSE(state.LoadFromFile("../test/non_exist.lua"));
    EXPECT_NE(capture.Content().find(expected), std::string::npos);

    EXPECT_EQ(state.Size(), 0);
}


static const std::string test_syntax_errror_script = R"(
function syntax_error()
    1 2 3 4
end
)";

TEST(Lua, load_syntax_error) 
{
    sel::State state(true);

    const char* expected = "unexpected symbol";
    CapturedStdout capture;
    EXPECT_FALSE(state.LoadFromString(test_syntax_errror_script));
    EXPECT_NE(capture.Content().find(expected), std::string::npos);

    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, do_syntax_error) 
{
    sel::State state(true);

    const char* expected = "unexpected symbol";
    CapturedStdout capture;
    bool b = state("function syntax_error() 1 2 3 4 end");
    EXPECT_FALSE(b);
    EXPECT_NE(capture.Content().find(expected), std::string::npos);

    EXPECT_EQ(state.Size(), 0);
}


static const std::string test_test_errror_script = R"(
function err_func1(x, y)
	err_func2(x + y)
end

function divide_by_zero()
	return 1 / 0
end

function _overflow(n)
	return _overflow(n + 1) + 1
end

function do_overflow()
	_overflow(1)
end

)";

TEST(Lua, call_undefined_function) 
{
    sel::State state(true);

    state.LoadFromString(test_test_errror_script);
    const char* expected = "attempt to call a nil value";
    CapturedStdout capture;
    state["undefined_function"]();
    EXPECT_NE(capture.Content().find(expected), std::string::npos);
    
    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, call_undefined_function2) 
{
    sel::State state(true);

    state.LoadFromString(test_test_errror_script);
#if LUA_VERSION_NUM < 503
    const char* expected = "attempt to call global 'err_func2'";
#else
    const char* expected = "attempt to call a nil value (global 'err_func2')";
#endif
    CapturedStdout capture;
    state["err_func1"](1, 2);
    EXPECT_NE(capture.Content().find(expected), std::string::npos);

    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, call_stackoverflow) 
{
    sel::State state(true);

    state.LoadFromString(test_test_errror_script);
    const char* expected = "stack overflow";
    CapturedStdout capture;
    state["do_overflow"]();
    auto co = capture.Content();
    EXPECT_NE(capture.Content().find(expected), std::string::npos);

    EXPECT_EQ(state.Size(), 0);
}

TEST(Lua, parameter_conversion_error) 
{
    sel::State state(true);

    const char * expected =
        "bad argument #2 to 'accept_string_int_string' (number expected, got string)";
    std::string largeStringToPreventSSO(50, 'x');
    state["accept_string_int_string"] = [](std::string, int, std::string){};

    CapturedStdout capture;
    state["accept_string_int_string"](
        largeStringToPreventSSO,
        "not a number",
        largeStringToPreventSSO);
    EXPECT_NE(capture.Content().find(expected), std::string::npos);

    EXPECT_EQ(state.Size(), 0);
}
