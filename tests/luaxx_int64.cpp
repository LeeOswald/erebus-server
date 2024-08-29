#include "common.hpp"

#include <erebus/luaxx/luaxx_int64.hxx>

static const std::string test_int64 = R"(
function add(a, b, result)
    local v = Er.getInt64(a) + Er.getInt64(b)
    Er.setInt64(result, v)
end
function sub(a, b, result)
    local v = Er.getInt64(a) - Er.getInt64(b)
    Er.setInt64(result, v)
end
function mul(a, b, result)
    local v = Er.getInt64(a) * Er.getInt64(b)
    Er.setInt64(result, v)
end
function div(a, b, result)
    local v = Er.getInt64(a) / Er.getInt64(b)
    Er.setInt64(result, v)
end
function mod(a, b, result)
    local v = Er.getInt64(a) % Er.getInt64(b)
    Er.setInt64(result, v)
end
function neg(a, result)
    local v = -Er.getInt64(a)
    Er.setInt64(result, v)
end
function band(a, b, result)
    local v = Er.getInt64(a) & Er.getInt64(b)
    Er.setInt64(result, v)
end
function bor(a, b, result)
    local v = Er.getInt64(a) | Er.getInt64(b)
    Er.setInt64(result, v)
end
function bxor(a, b, result)
    local v = Er.getInt64(a) ~ Er.getInt64(b)
    Er.setInt64(result, v)
end
function bnot(a, result)
    local v = ~Er.getInt64(a)
    Er.setInt64(result, v)
end
function shl(a, n, result)
    local v = Er.getInt64(a) << n
    Er.setInt64(result, v)
end
function shr(a, n, result)
    local v = Er.getInt64(a) >> n
    Er.setInt64(result, v)
end
function eq(a, b)
    return Er.getInt64(a) == Er.getInt64(b)
end
function neq(a, b)
    return Er.getInt64(a) ~= Er.getInt64(b)
end
function lt(a, b)
    return Er.getInt64(a) < Er.getInt64(b)
end
function gt(a, b)
    return Er.getInt64(a) > Er.getInt64(b)
end
function le(a, b)
    return Er.getInt64(a) <= Er.getInt64(b)
end
)";

static const std::string test_uint64 = R"(
function add(a, b, result)
    local v = Er.getUInt64(a) + Er.getUInt64(b)
    Er.setUInt64(result, v)
end
function sub(a, b, result)
    local v = Er.getUInt64(a) - Er.getUInt64(b)
    Er.setUInt64(result, v)
end
function mul(a, b, result)
    local v = Er.getUInt64(a) * Er.getUInt64(b)
    Er.setUInt64(result, v)
end
function div(a, b, result)
    local v = Er.getUInt64(a) / Er.getUInt64(b)
    Er.setUInt64(result, v)
end
function mod(a, b, result)
    local v = Er.getUInt64(a) % Er.getUInt64(b)
    Er.setUInt64(result, v)
end
function neg(a, result)
    local v = -Er.getUInt64(a)
    Er.setUInt64(result, v)
end
function band(a, b, result)
    local v = Er.getUInt64(a) & Er.getUInt64(b)
    Er.setUInt64(result, v)
end
function bor(a, b, result)
    local v = Er.getUInt64(a) | Er.getUInt64(b)
    Er.setUInt64(result, v)
end
function bxor(a, b, result)
    local v = Er.getUInt64(a) ~ Er.getUInt64(b)
    Er.setUInt64(result, v)
end
function bnot(a, result)
    local v = ~Er.getUInt64(a)
    Er.setUInt64(result, v)
end
function shl(a, n, result)
    local v = Er.getUInt64(a) << n
    Er.setUInt64(result, v)
end
function shr(a, n, result)
    local v = Er.getUInt64(a) >> n
    Er.setUInt64(result, v)
end
function eq(a, b)
    return Er.getUInt64(a) == Er.getUInt64(b)
end
function neq(a, b)
    return Er.getUInt64(a) ~= Er.getUInt64(b)
end
function lt(a, b)
    return Er.getUInt64(a) < Er.getUInt64(b)
end
function gt(a, b)
    return Er.getUInt64(a) > Er.getUInt64(b)
end
function le(a, b)
    return Er.getUInt64(a) <= Er.getUInt64(b)
end
)";


TEST(Er_Lua, Int64)
{
    Er::LuaState state(g_log);
    
    state.loadString(test_int64, "test_int64");

    // +
    {
        Er::Lua::Int64Wrapper a(0x4000000050000001LL);
        Er::Lua::Int64Wrapper b(0x0000000000000002LL);

        Er::Lua::Int64Wrapper res; 
        state["add"](a, b, &res);
        
        EXPECT_EQ(res.value, 0x4000000050000003LL);
    }

    // -
    {
        Er::Lua::Int64Wrapper a(0x4000000050000003LL);
        Er::Lua::Int64Wrapper b(0x0000000000000002LL);

        Er::Lua::Int64Wrapper res;
        state["sub"](a, b, &res);

        EXPECT_EQ(res.value, 0x4000000050000001LL);
    }

    // *
    {
        Er::Lua::Int64Wrapper a(3);
        Er::Lua::Int64Wrapper b(-2);

        Er::Lua::Int64Wrapper res;
        state["mul"](a, b, &res);

        EXPECT_EQ(res.value, -6);
    }

    // /
    {
        Er::Lua::Int64Wrapper a(6);
        Er::Lua::Int64Wrapper b(-2);

        Er::Lua::Int64Wrapper res;
        state["div"](a, b, &res);

        EXPECT_EQ(res.value, -3);
    }

    // %
    {
        Er::Lua::Int64Wrapper a(7);
        Er::Lua::Int64Wrapper b(2);

        Er::Lua::Int64Wrapper res;
        state["mod"](a, b, &res);

        EXPECT_EQ(res.value, 1);
    }

    // -x
    {
        Er::Lua::Int64Wrapper a(7);

        Er::Lua::Int64Wrapper res;
        state["neg"](a, &res);

        EXPECT_EQ(res.value, -7);
    }

    // &
    {
        Er::Lua::Int64Wrapper a(0x8000000100100001LL);
        Er::Lua::Int64Wrapper b(0x0000000100100000LL);

        Er::Lua::Int64Wrapper res;
        state["band"](a, b, &res);

        EXPECT_EQ(res.value, 0x0000000100100000LL);
    }

    // |
    {
        Er::Lua::Int64Wrapper a(0x8000000100100001LL);
        Er::Lua::Int64Wrapper b(0x0000000100100000LL);

        Er::Lua::Int64Wrapper res;
        state["bor"](a, b, &res);

        EXPECT_EQ(res.value, 0x8000000100100001LL);
    }

    // ^
    {
        Er::Lua::Int64Wrapper a(0x8000000100100001LL);
        Er::Lua::Int64Wrapper b(0x0000000100100000LL);

        Er::Lua::Int64Wrapper res;
        state["bxor"](a, b, &res);

        EXPECT_EQ(res.value, 0x8000000000000001LL);
    }

    // ~x
    {
        Er::Lua::Int64Wrapper a(0x8000000100100001LL);

        Er::Lua::Int64Wrapper res;
        state["bnot"](a, &res);

        EXPECT_EQ(res.value, 0x7FFFFFFEFFEFFFFELL);
    }

    // <<
    {
        Er::Lua::Int64Wrapper a(0x0000000100100001LL);

        Er::Lua::Int64Wrapper res;
        state["shl"](a, 2, &res);

        EXPECT_EQ(res.value, 0x400400004LL);
    }

    // >>
    {
        Er::Lua::Int64Wrapper a(0x0000000100100001LL);

        Er::Lua::Int64Wrapper res;
        state["shr"](a, 2, &res);

        EXPECT_EQ(res.value, 0x40040000LL);
    }

    // ==
    {
        Er::Lua::Int64Wrapper a(0x8000000100100001LL);
        Er::Lua::Int64Wrapper b(0x0000000100100000LL);

        bool eq = state["eq"](a, b);
        EXPECT_FALSE(eq);

        eq = state["eq"](a, a);
        EXPECT_TRUE(eq);
    }

    // !=
    {
        Er::Lua::Int64Wrapper a(0x8000000100100001LL);
        Er::Lua::Int64Wrapper b(0x0000000100100000LL);

        bool neq = state["neq"](a, b);
        EXPECT_TRUE(neq);

        neq = state["neq"](a, a);
        EXPECT_FALSE(neq);
    }

    // <
    {
        Er::Lua::Int64Wrapper a(8);
        Er::Lua::Int64Wrapper b(-10);

        bool lt = state["lt"](a, b);
        EXPECT_FALSE(lt);

        lt = state["lt"](b, a);
        EXPECT_TRUE(lt);
    }

    // >
    {
        Er::Lua::Int64Wrapper a(8);
        Er::Lua::Int64Wrapper b(-10);

        bool gt = state["gt"](a, b);
        EXPECT_TRUE(gt);

        gt = state["gt"](b, a);
        EXPECT_FALSE(gt);
    }

    // <
    {
        Er::Lua::Int64Wrapper a(8);
        Er::Lua::Int64Wrapper b(-10);

        bool le = state["le"](a, b);
        EXPECT_FALSE(le);

        le = state["le"](b, a);
        EXPECT_TRUE(le);

        le = state["le"](a, a);
        EXPECT_TRUE(le);
    }
}

TEST(Er_Lua, UInt64)
{
    Er::LuaState state(g_log);

    state.loadString(test_uint64, "test_uint64");

    // +
    {
        Er::Lua::UInt64Wrapper a(0x4000000050000001ULL);
        Er::Lua::UInt64Wrapper b(0x0000000000000002ULL);

        Er::Lua::UInt64Wrapper res;
        state["add"](a, b, &res);

        EXPECT_EQ(res.value, 0x4000000050000003ULL);
    }

    // -
    {
        Er::Lua::UInt64Wrapper a(0x4000000050000003ULL);
        Er::Lua::UInt64Wrapper b(0x0000000000000002ULL);

        Er::Lua::UInt64Wrapper res;
        state["sub"](a, b, &res);

        EXPECT_EQ(res.value, 0x4000000050000001ULL);
    }

    // *
    {
        Er::Lua::UInt64Wrapper a(3);
        Er::Lua::UInt64Wrapper b(2);

        Er::Lua::UInt64Wrapper res;
        state["mul"](a, b, &res);

        EXPECT_EQ(res.value, 6);
    }

    // /
    {
        Er::Lua::UInt64Wrapper a(6);
        Er::Lua::UInt64Wrapper b(2);

        Er::Lua::UInt64Wrapper res;
        state["div"](a, b, &res);

        EXPECT_EQ(res.value, 3);
    }

    // %
    {
        Er::Lua::UInt64Wrapper a(7);
        Er::Lua::UInt64Wrapper b(2);

        Er::Lua::UInt64Wrapper res;
        state["mod"](a, b, &res);

        EXPECT_EQ(res.value, 1);
    }

    // -x
    {
        Er::Lua::UInt64Wrapper a(7);

        Er::Lua::UInt64Wrapper res;
        state["neg"](a, &res);

        EXPECT_EQ(res.value, uint64_t(-7));
    }

    // &
    {
        Er::Lua::UInt64Wrapper a(0x8000000100100001ULL);
        Er::Lua::UInt64Wrapper b(0x0000000100100000ULL);

        Er::Lua::UInt64Wrapper res;
        state["band"](a, b, &res);

        EXPECT_EQ(res.value, 0x0000000100100000ULL);
    }

    // |
    {
        Er::Lua::UInt64Wrapper a(0x8000000100100001ULL);
        Er::Lua::UInt64Wrapper b(0x0000000100100000ULL);

        Er::Lua::UInt64Wrapper res;
        state["bor"](a, b, &res);

        EXPECT_EQ(res.value, 0x8000000100100001ULL);
    }

    // ^
    {
        Er::Lua::UInt64Wrapper a(0x8000000100100001ULL);
        Er::Lua::UInt64Wrapper b(0x0000000100100000ULL);

        Er::Lua::UInt64Wrapper res;
        state["bxor"](a, b, &res);

        EXPECT_EQ(res.value, 0x8000000000000001ULL);
    }

    // ~x
    {
        Er::Lua::UInt64Wrapper a(0x8000000100100001ULL);

        Er::Lua::UInt64Wrapper res;
        state["bnot"](a, &res);

        EXPECT_EQ(res.value, 0x7FFFFFFEFFEFFFFEULL);
    }

    // <<
    {
        Er::Lua::UInt64Wrapper a(0x0000000100100001ULL);

        Er::Lua::UInt64Wrapper res;
        state["shl"](a, 2, &res);

        EXPECT_EQ(res.value, 0x400400004ULL);
    }

    // >>
    {
        Er::Lua::UInt64Wrapper a(0x0000000100100001ULL);

        Er::Lua::UInt64Wrapper res;
        state["shr"](a, 2, &res);

        EXPECT_EQ(res.value, 0x40040000ULL);
    }

    // ==
    {
        Er::Lua::UInt64Wrapper a(0x8000000100100001ULL);
        Er::Lua::UInt64Wrapper b(0x0000000100100000ULL);

        bool eq = state["eq"](a, b);
        EXPECT_FALSE(eq);

        eq = state["eq"](a, a);
        EXPECT_TRUE(eq);
    }

    // !=
    {
        Er::Lua::UInt64Wrapper a(0x8000000100100001ULL);
        Er::Lua::UInt64Wrapper b(0x0000000100100000ULL);

        bool neq = state["neq"](a, b);
        EXPECT_TRUE(neq);

        neq = state["neq"](a, a);
        EXPECT_FALSE(neq);
    }

    // <
    {
        Er::Lua::UInt64Wrapper a(8);
        Er::Lua::UInt64Wrapper b(5);

        bool lt = state["lt"](a, b);
        EXPECT_FALSE(lt);

        lt = state["lt"](b, a);
        EXPECT_TRUE(lt);
    }

    // >
    {
        Er::Lua::UInt64Wrapper a(8);
        Er::Lua::UInt64Wrapper b(2);

        bool gt = state["gt"](a, b);
        EXPECT_TRUE(gt);

        gt = state["gt"](b, a);
        EXPECT_FALSE(gt);
    }

    // <
    {
        Er::Lua::UInt64Wrapper a(8);
        Er::Lua::UInt64Wrapper b(4);

        bool le = state["le"](a, b);
        EXPECT_FALSE(le);

        le = state["le"](b, a);
        EXPECT_TRUE(le);

        le = state["le"](a, a);
        EXPECT_TRUE(le);
    }
}