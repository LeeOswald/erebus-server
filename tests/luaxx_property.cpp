#include "common.hpp"

#include <erebus/luaxx/luaxx_tuple.hxx>

TEST(Er_Lua, PropertyTypes)
{
    Er::LuaState state(g_log);

    uint32_t type = state["Er"]["PropertyType"]["Invalid"];
    EXPECT_EQ(type, static_cast<uint32_t>(Er::PropertyType::Invalid));

    type = state["Er"]["PropertyType"]["Empty"];
    EXPECT_EQ(type, static_cast<uint32_t>(Er::PropertyType::Empty));

    type = state["Er"]["PropertyType"]["Bool"];
    EXPECT_EQ(type, static_cast<uint32_t>(Er::PropertyType::Bool));

    type = state["Er"]["PropertyType"]["Int32"];
    EXPECT_EQ(type, static_cast<uint32_t>(Er::PropertyType::Int32));

    type = state["Er"]["PropertyType"]["UInt32"];
    EXPECT_EQ(type, static_cast<uint32_t>(Er::PropertyType::UInt32));

    type = state["Er"]["PropertyType"]["Int64"];
    EXPECT_EQ(type, static_cast<uint32_t>(Er::PropertyType::Int64));

    type = state["Er"]["PropertyType"]["UInt64"];
    EXPECT_EQ(type, static_cast<uint32_t>(Er::PropertyType::UInt64));

    type = state["Er"]["PropertyType"]["Double"];
    EXPECT_EQ(type, static_cast<uint32_t>(Er::PropertyType::Double));

    type = state["Er"]["PropertyType"]["String"];
    EXPECT_EQ(type, static_cast<uint32_t>(Er::PropertyType::String));

    type = state["Er"]["PropertyType"]["Bytes"];
    EXPECT_EQ(type, static_cast<uint32_t>(Er::PropertyType::Bytes));
}

TEST(Er_Lua, registerLuaProps)
{
    Er::LuaState state(g_log);
    TestProps::registerLuaProps(state);

    {
        uint32_t id = state["TestProps"]["BoolProp"]["id"];
        EXPECT_EQ(id, TestProps::BoolProp::id());
        std::string id_str = state["TestProps"]["BoolProp"]["id_str"];
        EXPECT_STREQ(id_str.c_str(), TestProps::BoolProp::id_str());
        uint32_t type = state["TestProps"]["BoolProp"]["type"];
        EXPECT_EQ(type, static_cast<uint32_t>(TestProps::BoolProp::type()));
    }
    {
        uint32_t id = state["TestProps"]["Int32Prop"]["id"];
        EXPECT_EQ(id, TestProps::Int32Prop::id());
        std::string id_str = state["TestProps"]["Int32Prop"]["id_str"];
        EXPECT_STREQ(id_str.c_str(), TestProps::Int32Prop::id_str());
        uint32_t type = state["TestProps"]["Int32Prop"]["type"];
        EXPECT_EQ(type, static_cast<uint32_t>(TestProps::Int32Prop::type()));
    }
    {
        uint32_t id = state["TestProps"]["UInt32Prop"]["id"];
        EXPECT_EQ(id, TestProps::UInt32Prop::id());
        std::string id_str = state["TestProps"]["UInt32Prop"]["id_str"];
        EXPECT_STREQ(id_str.c_str(), TestProps::UInt32Prop::id_str());
        uint32_t type = state["TestProps"]["UInt32Prop"]["type"];
        EXPECT_EQ(type, static_cast<uint32_t>(TestProps::UInt32Prop::type()));
    }
    {
        uint32_t id = state["TestProps"]["Int64Prop"]["id"];
        EXPECT_EQ(id, TestProps::Int64Prop::id());
        std::string id_str = state["TestProps"]["Int64Prop"]["id_str"];
        EXPECT_STREQ(id_str.c_str(), TestProps::Int64Prop::id_str());
        uint32_t type = state["TestProps"]["Int64Prop"]["type"];
        EXPECT_EQ(type, static_cast<uint32_t>(TestProps::Int64Prop::type()));
    }
    {
        uint32_t id = state["TestProps"]["UInt64Prop"]["id"];
        EXPECT_EQ(id, TestProps::UInt64Prop::id());
        std::string id_str = state["TestProps"]["UInt64Prop"]["id_str"];
        EXPECT_STREQ(id_str.c_str(), TestProps::UInt64Prop::id_str());
        uint32_t type = state["TestProps"]["UInt64Prop"]["type"];
        EXPECT_EQ(type, static_cast<uint32_t>(TestProps::UInt64Prop::type()));
    }
    {
        uint32_t id = state["TestProps"]["DoubleProp"]["id"];
        EXPECT_EQ(id, TestProps::DoubleProp::id());
        std::string id_str = state["TestProps"]["DoubleProp"]["id_str"];
        EXPECT_STREQ(id_str.c_str(), TestProps::DoubleProp::id_str());
        uint32_t type = state["TestProps"]["DoubleProp"]["type"];
        EXPECT_EQ(type, static_cast<uint32_t>(TestProps::DoubleProp::type()));
    }
    {
        uint32_t id = state["TestProps"]["StringProp"]["id"];
        EXPECT_EQ(id, TestProps::StringProp::id());
        std::string id_str = state["TestProps"]["StringProp"]["id_str"];
        EXPECT_STREQ(id_str.c_str(), TestProps::StringProp::id_str());
        uint32_t type = state["TestProps"]["StringProp"]["type"];
        EXPECT_EQ(type, static_cast<uint32_t>(TestProps::StringProp::type()));
    }
    {
        uint32_t id = state["TestProps"]["BytesProp"]["id"];
        EXPECT_EQ(id, TestProps::BytesProp::id());
        std::string id_str = state["TestProps"]["BytesProp"]["id_str"];
        EXPECT_STREQ(id_str.c_str(), TestProps::BytesProp::id_str());
        uint32_t type = state["TestProps"]["BytesProp"]["type"];
        EXPECT_EQ(type, static_cast<uint32_t>(TestProps::BytesProp::type()));
    }
}



static const std::string test_property_adapter = R"(
function filter(ev)
    local ty = Er.Property.getType(ev)
    local id = Er.Property.getId(ev)

    if id == TestProps.BoolProp.id then
        local v = Er.Property.getBool(ev)
        v = not v
        Er.Property.setBool(ev, v)
    elseif id == TestProps.Int32Prop.id then
        local v = Er.Property.getInt32(ev)
        v = v + 1
        Er.Property.setInt32(ev, v)
    elseif id == TestProps.UInt32Prop.id then
        local v = Er.Property.getUInt32(ev)
        v = v + 2
        Er.Property.setUInt32(ev, v)
    elseif id == TestProps.Int64Prop.id then
        local v = Er.Property.getInt64(ev)
        v = v + Er.Int64.new(0, 3)
        Er.Property.setInt64(ev, v)
    elseif id == TestProps.UInt64Prop.id then
        local v = Er.Property.getUInt64(ev)
        v = v + Er.UInt64.new(0, 4)
        Er.Property.setUInt64(ev, v)
    elseif id == TestProps.DoubleProp.id then
        local v = Er.Property.getDouble(ev)
        v = v + 5
        Er.Property.setDouble(ev, v)
    elseif id == TestProps.StringProp.id then
        local v = Er.Property.getString(ev)
        v = string.upper(v)
        Er.Property.setString(ev, v)
    elseif id == TestProps.BytesProp.id then
        local v = Er.Property.getBytes(ev)
        v = string.lower(v)
        Er.Property.setBytes(ev, v)
    end

    return ty
end
)";

TEST(Er_Lua, Property)
{
    Er::LuaState state(g_log);
    TestProps::registerLuaProps(state);

    state.loadString(test_property_adapter, "test_property_adapter");

    {
        Er::Property prop(TestProps::BoolProp::id(), false);

        uint32_t type = state["filter"](&prop);
        EXPECT_EQ(prop.id, TestProps::BoolProp::id());
        ASSERT_EQ(type, static_cast<uint32_t>(TestProps::BoolProp::type()));
        ASSERT_EQ(prop.type(), TestProps::BoolProp::type());
        EXPECT_EQ(std::get<bool>(prop.value), true);
    }
    {
        Er::Property prop(TestProps::Int32Prop::id(), int32_t(133));

        uint32_t type = state["filter"](&prop);
        EXPECT_EQ(prop.id, TestProps::Int32Prop::id());
        ASSERT_EQ(type, static_cast<uint32_t>(TestProps::Int32Prop::type()));
        ASSERT_EQ(prop.type(), TestProps::Int32Prop::type());
        EXPECT_EQ(std::get<int32_t>(prop.value), 133 + 1);
    }
    {
        Er::Property prop(TestProps::UInt32Prop::id(), uint32_t(133));

        uint32_t type = state["filter"](&prop);
        EXPECT_EQ(prop.id, TestProps::UInt32Prop::id());
        ASSERT_EQ(type, static_cast<uint32_t>(TestProps::UInt32Prop::type()));
        ASSERT_EQ(prop.type(), TestProps::UInt32Prop::type());
        EXPECT_EQ(std::get<uint32_t>(prop.value), 133 + 2);
    }
    {
        Er::Property prop(TestProps::Int64Prop::id(), int64_t(133));

        uint32_t type = state["filter"](&prop);
        EXPECT_EQ(prop.id, TestProps::Int64Prop::id());
        ASSERT_EQ(type, static_cast<uint32_t>(TestProps::Int64Prop::type()));
        ASSERT_EQ(prop.type(), TestProps::Int64Prop::type());
        EXPECT_EQ(std::get<int64_t>(prop.value), 133 + 3);
    }
    {
        Er::Property prop(TestProps::UInt64Prop::id(), uint64_t(133));

        uint32_t type = state["filter"](&prop);
        EXPECT_EQ(prop.id, TestProps::UInt64Prop::id());
        ASSERT_EQ(type, static_cast<uint32_t>(TestProps::UInt64Prop::type()));
        ASSERT_EQ(prop.type(), TestProps::UInt64Prop::type());
        EXPECT_EQ(std::get<uint64_t>(prop.value), 133 + 4);
    }
    {
        Er::Property prop(TestProps::DoubleProp::id(), double(133));

        uint32_t type = state["filter"](&prop);
        EXPECT_EQ(prop.id, TestProps::DoubleProp::id());
        ASSERT_EQ(type, static_cast<uint32_t>(TestProps::DoubleProp::type()));
        ASSERT_EQ(prop.type(), TestProps::DoubleProp::type());
        EXPECT_EQ(std::get<double>(prop.value), 133 + 5);
    }
    {
        Er::Property prop(TestProps::StringProp::id(), std::string("test_string"));

        uint32_t type = state["filter"](&prop);
        EXPECT_EQ(prop.id, TestProps::StringProp::id());
        ASSERT_EQ(type, static_cast<uint32_t>(TestProps::StringProp::type()));
        ASSERT_EQ(prop.type(), TestProps::StringProp::type());
        EXPECT_STREQ(std::get<std::string>(prop.value).c_str(), "TEST_STRING");
    }
    {
        Er::Property prop(TestProps::BytesProp::id(), Er::Bytes(std::string("BYTES_TEST")));

        uint32_t type = state["filter"](&prop);
        EXPECT_EQ(prop.id, TestProps::BytesProp::id());
        ASSERT_EQ(type, static_cast<uint32_t>(TestProps::BytesProp::type()));
        ASSERT_EQ(prop.type(), TestProps::BytesProp::type());
        EXPECT_STREQ(std::get<Er::Bytes>(prop.value).bytes().c_str(), "bytes_test");
    }
}

static const std::string test_property_info = R"(
local DOMAIN = "Test"
function lookup(id)
    local pi = Er.PropertyInfo.new(DOMAIN, id)
    if not pi:valid() then
        return false
    end
    return pi:valid(), id, pi:id(), pi:type(), pi:id_str(), pi:name()
end
)";

TEST(Er_Lua, PropertyInfo)
{
    Er::LuaState state(g_log);
    TestProps::registerLuaProps(state);

    state.loadString(test_property_info, "test_property_info");

    {
        bool valid = false;
        uint32_t id = Er::InvalidPropId;
        uint32_t id1 = Er::InvalidPropId;
        uint32_t type = static_cast<uint32_t>(Er::PropertyType::Invalid);
        std::string id_str;
        std::string name;
        Er::Lua::tie(valid, id, id1, type, id_str, name) = state["lookup"](uint32_t(Er::InvalidPropId));
        EXPECT_FALSE(valid);
    }
    
    {
        bool valid = false;
        uint32_t id = Er::InvalidPropId;
        uint32_t id1 = Er::InvalidPropId;
        uint32_t type = static_cast<uint32_t>(Er::PropertyType::Invalid);
        std::string id_str;
        std::string name;
        Er::Lua::tie(valid, id, id1, type, id_str, name) = state["lookup"](uint32_t(TestProps::BoolProp::id()));
        ASSERT_TRUE(valid);
        EXPECT_EQ(id, uint32_t(TestProps::BoolProp::id()));
        EXPECT_EQ(id1, uint32_t(TestProps::BoolProp::id()));
        EXPECT_EQ(type, uint32_t(TestProps::BoolProp::type()));
        EXPECT_STREQ(id_str.c_str(), TestProps::BoolProp::id_str());
        EXPECT_STREQ(name.c_str(), TestProps::BoolProp::name());
    }

    {
        bool valid = false;
        uint32_t id = Er::InvalidPropId;
        uint32_t id1 = Er::InvalidPropId;
        uint32_t type = static_cast<uint32_t>(Er::PropertyType::Invalid);
        std::string id_str;
        std::string name;
        Er::Lua::tie(valid, id, id1, type, id_str, name) = state["lookup"](uint32_t(TestProps::Int32Prop::id()));
        ASSERT_TRUE(valid);
        EXPECT_EQ(id, uint32_t(TestProps::Int32Prop::id()));
        EXPECT_EQ(id1, uint32_t(TestProps::Int32Prop::id()));
        EXPECT_EQ(type, uint32_t(TestProps::Int32Prop::type()));
        EXPECT_STREQ(id_str.c_str(), TestProps::Int32Prop::id_str());
        EXPECT_STREQ(name.c_str(), TestProps::Int32Prop::name());
    }

    {
        bool valid = false;
        uint32_t id = Er::InvalidPropId;
        uint32_t id1 = Er::InvalidPropId;
        uint32_t type = static_cast<uint32_t>(Er::PropertyType::Invalid);
        std::string id_str;
        std::string name;
        Er::Lua::tie(valid, id, id1, type, id_str, name) = state["lookup"](uint32_t(TestProps::UInt32Prop::id()));
        ASSERT_TRUE(valid);
        EXPECT_EQ(id, uint32_t(TestProps::UInt32Prop::id()));
        EXPECT_EQ(id1, uint32_t(TestProps::UInt32Prop::id()));
        EXPECT_EQ(type, uint32_t(TestProps::UInt32Prop::type()));
        EXPECT_STREQ(id_str.c_str(), TestProps::UInt32Prop::id_str());
        EXPECT_STREQ(name.c_str(), TestProps::UInt32Prop::name());
    }

    {
        bool valid = false;
        uint32_t id = Er::InvalidPropId;
        uint32_t id1 = Er::InvalidPropId;
        uint32_t type = static_cast<uint32_t>(Er::PropertyType::Invalid);
        std::string id_str;
        std::string name;
        Er::Lua::tie(valid, id, id1, type, id_str, name) = state["lookup"](uint32_t(TestProps::Int64Prop::id()));
        ASSERT_TRUE(valid);
        EXPECT_EQ(id, uint32_t(TestProps::Int64Prop::id()));
        EXPECT_EQ(id1, uint32_t(TestProps::Int64Prop::id()));
        EXPECT_EQ(type, uint32_t(TestProps::Int64Prop::type()));
        EXPECT_STREQ(id_str.c_str(), TestProps::Int64Prop::id_str());
        EXPECT_STREQ(name.c_str(), TestProps::Int64Prop::name());
    }

    {
        bool valid = false;
        uint32_t id = Er::InvalidPropId;
        uint32_t id1 = Er::InvalidPropId;
        uint32_t type = static_cast<uint32_t>(Er::PropertyType::Invalid);
        std::string id_str;
        std::string name;
        Er::Lua::tie(valid, id, id1, type, id_str, name) = state["lookup"](uint32_t(TestProps::UInt64Prop::id()));
        ASSERT_TRUE(valid);
        EXPECT_EQ(id, uint32_t(TestProps::UInt64Prop::id()));
        EXPECT_EQ(id1, uint32_t(TestProps::UInt64Prop::id()));
        EXPECT_EQ(type, uint32_t(TestProps::UInt64Prop::type()));
        EXPECT_STREQ(id_str.c_str(), TestProps::UInt64Prop::id_str());
        EXPECT_STREQ(name.c_str(), TestProps::UInt64Prop::name());
    }

    {
        bool valid = false;
        uint32_t id = Er::InvalidPropId;
        uint32_t id1 = Er::InvalidPropId;
        uint32_t type = static_cast<uint32_t>(Er::PropertyType::Invalid);
        std::string id_str;
        std::string name;
        Er::Lua::tie(valid, id, id1, type, id_str, name) = state["lookup"](uint32_t(TestProps::DoubleProp::id()));
        ASSERT_TRUE(valid);
        EXPECT_EQ(id, uint32_t(TestProps::DoubleProp::id()));
        EXPECT_EQ(id1, uint32_t(TestProps::DoubleProp::id()));
        EXPECT_EQ(type, uint32_t(TestProps::DoubleProp::type()));
        EXPECT_STREQ(id_str.c_str(), TestProps::DoubleProp::id_str());
        EXPECT_STREQ(name.c_str(), TestProps::DoubleProp::name());
    }

    {
        bool valid = false;
        uint32_t id = Er::InvalidPropId;
        uint32_t id1 = Er::InvalidPropId;
        uint32_t type = static_cast<uint32_t>(Er::PropertyType::Invalid);
        std::string id_str;
        std::string name;
        Er::Lua::tie(valid, id, id1, type, id_str, name) = state["lookup"](uint32_t(TestProps::StringProp::id()));
        ASSERT_TRUE(valid);
        EXPECT_EQ(id, uint32_t(TestProps::StringProp::id()));
        EXPECT_EQ(id1, uint32_t(TestProps::StringProp::id()));
        EXPECT_EQ(type, uint32_t(TestProps::StringProp::type()));
        EXPECT_STREQ(id_str.c_str(), TestProps::StringProp::id_str());
        EXPECT_STREQ(name.c_str(), TestProps::StringProp::name());
    }

    {
        bool valid = false;
        uint32_t id = Er::InvalidPropId;
        uint32_t id1 = Er::InvalidPropId;
        uint32_t type = static_cast<uint32_t>(Er::PropertyType::Invalid);
        std::string id_str;
        std::string name;
        Er::Lua::tie(valid, id, id1, type, id_str, name) = state["lookup"](uint32_t(TestProps::BytesProp::id()));
        ASSERT_TRUE(valid);
        EXPECT_EQ(id, uint32_t(TestProps::BytesProp::id()));
        EXPECT_EQ(id1, uint32_t(TestProps::BytesProp::id()));
        EXPECT_EQ(type, uint32_t(TestProps::BytesProp::type()));
        EXPECT_STREQ(id_str.c_str(), TestProps::BytesProp::id_str());
        EXPECT_STREQ(name.c_str(), TestProps::BytesProp::name());
    }
}

static const std::string test_property_format = R"(
local DOMAIN = "Test"
function format(prop)
    local id = Er.Property.getId(prop)
    local pi = Er.PropertyInfo.new(DOMAIN, id)
    if not pi:valid() then
        return ""
    end
    return pi:format(prop)
end
)";


TEST(Er_Lua, FormatProperty)
{
    Er::LuaState state(g_log);
    TestProps::registerLuaProps(state);

    state.loadString(test_property_format, "test_property_format");

    {
        Er::Property prop(TestProps::BoolProp::id(), true);

        std::string str = state["format"](&prop);
        EXPECT_STREQ(str.c_str(), "true");
    }

    {
        Er::Property prop(TestProps::Int32Prop::id(), int32_t(-12));

        std::string str = state["format"](&prop);
        EXPECT_STREQ(str.c_str(), "-12");
    }
    
    {
        Er::Property prop(TestProps::UInt32Prop::id(), uint32_t(122));

        std::string str = state["format"](&prop);
        EXPECT_STREQ(str.c_str(), "122");
    }

    {
        Er::Property prop(TestProps::Int64Prop::id(), int64_t(-1299));

        std::string str = state["format"](&prop);
        EXPECT_STREQ(str.c_str(), "-1299");
    }

    {
        Er::Property prop(TestProps::UInt64Prop::id(), uint64_t(4097));

        std::string str = state["format"](&prop);
        EXPECT_STREQ(str.c_str(), "4097");
    }

    {
        Er::Property prop(TestProps::DoubleProp::id(), double(-8.8));

        std::string str = state["format"](&prop);
        EXPECT_STREQ(str.c_str(), "-8.800");
    }

    {
        Er::Property prop(TestProps::StringProp::id(), std::string("test_string"));

        std::string str = state["format"](&prop);
        EXPECT_STREQ(str.c_str(), "test_string");
    }
}
