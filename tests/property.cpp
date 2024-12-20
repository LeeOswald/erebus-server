#include "common.hpp"

#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>


TEST(Er_Property, registration)
{
    using SomeProp = Er::PropertyValue<Er::Bool, ER_PROPID("some_test_prop"), "SomeBool", Er::PropertyFormatter<Er::Bool>>;
    Er::registerProperty("Test", SomeProp::make_info(), Er::Log::defaultLog());
    
    auto somePropById = Er::lookupProperty("Test", SomeProp::Id::value);
    ASSERT_TRUE(somePropById);
    auto somePropByStrId = Er::lookupProperty("Test", SomeProp::id_str());
    ASSERT_TRUE(somePropByStrId);
    EXPECT_EQ(somePropById.get(), somePropByStrId.get());
    EXPECT_EQ(somePropById->type(), Er::PropertyType::Bool);
    EXPECT_EQ(somePropById->id(), SomeProp::Id::value);
    EXPECT_STREQ(somePropById->id_str(), "some_test_prop");
    EXPECT_STREQ(somePropById->name(), "SomeBool");

    Er::unregisterProperty("Test", Er::lookupProperty("Test", SomeProp::Id::value), Er::Log::defaultLog());
    auto no = Er::lookupProperty("Test", SomeProp::Id::value);
    EXPECT_FALSE(no);
}

TEST(Er_Property, constructFromPropertyValue)
{
    {
        TestProps::BoolProp bf(Er::False);
        Er::Property propf(bf);
        EXPECT_EQ(propf.id, TestProps::BoolProp::Id::value);
        EXPECT_EQ(propf.type(), Er::PropertyType::Bool);
        EXPECT_EQ(Er::get<Er::Bool>(propf.value), Er::False);

        Er::Property propt{ TestProps::BoolProp(Er::True) };
        EXPECT_EQ(propt.id, TestProps::BoolProp::Id::value);
        EXPECT_EQ(propt.type(), Er::PropertyType::Bool);
        EXPECT_EQ(Er::get<Er::Bool>(propt.value), Er::True);

        auto info = Er::lookupProperty("Test", propt.id);
        ASSERT_TRUE(info);
        EXPECT_NE(propf, propt);
        EXPECT_EQ(Er::Property(TestProps::BoolProp(Er::True)), Er::Property(TestProps::BoolProp(Er::True)));
        EXPECT_NE(Er::Property(TestProps::BoolProp(Er::True)), Er::Property(TestProps::Int32Prop(1)));
        EXPECT_EQ(info->id(), TestProps::BoolProp::Id::value);
        EXPECT_STREQ(info->id_str(), bf.id_str());
        EXPECT_STREQ(info->name(), bf.name());
        EXPECT_EQ(info->type(), bf.type());
        
        Er::Property propz;
        EXPECT_NE(propt, propz);
    }

    {
        TestProps::Int32Prop b0(0);
        Er::Property prop0(b0);
        EXPECT_EQ(prop0.id, TestProps::Int32Prop::Id::value);
        EXPECT_EQ(prop0.type(), Er::PropertyType::Int32);
        EXPECT_EQ(Er::get<int32_t>(prop0.value), 0);

        Er::Property prop2(TestProps::Int32Prop(-2));
        EXPECT_EQ(prop2.id, TestProps::Int32Prop::Id::value);
        EXPECT_EQ(prop2.type(), Er::PropertyType::Int32);
        EXPECT_EQ(Er::get<int32_t>(prop2.value), -2);

        auto info = Er::lookupProperty("Test", prop2.id);
        ASSERT_TRUE(info);
        EXPECT_NE(prop0, prop2);
        EXPECT_EQ(Er::Property(TestProps::Int32Prop(3)), Er::Property(TestProps::Int32Prop(3)));
        EXPECT_NE(Er::Property(TestProps::Int32Prop(3)), Er::Property(TestProps::UInt32Prop(3)));
        EXPECT_EQ(info->id(), TestProps::Int32Prop::Id::value);
        EXPECT_STREQ(info->id_str(), b0.id_str());
        EXPECT_STREQ(info->name(), b0.name());
        EXPECT_EQ(info->type(), b0.type());
        
        Er::Property propz;
        EXPECT_NE(prop2, propz);
    }

    {
        TestProps::UInt32Prop b0(0);
        Er::Property prop0(b0);
        EXPECT_EQ(prop0.id, TestProps::UInt32Prop::Id::value);
        EXPECT_EQ(prop0.type(), Er::PropertyType::UInt32);
        EXPECT_EQ(Er::get<uint32_t>(prop0.value), 0);

        Er::Property prop2(TestProps::UInt32Prop(2));
        EXPECT_EQ(prop2.id, TestProps::UInt32Prop::Id::value);
        EXPECT_EQ(prop2.type(), Er::PropertyType::UInt32);
        EXPECT_EQ(Er::get<uint32_t>(prop2.value), 2);

        auto info = Er::lookupProperty("Test", prop2.id);
        ASSERT_TRUE(info);
        EXPECT_NE(prop0, prop2);
        EXPECT_EQ(Er::Property(TestProps::UInt32Prop(5)), Er::Property(TestProps::UInt32Prop(5)));
        EXPECT_NE(Er::Property(TestProps::UInt32Prop(5)), Er::Property(TestProps::Int32Prop(5)));
        EXPECT_EQ(info->id(), TestProps::UInt32Prop::Id::value);
        EXPECT_STREQ(info->id_str(), b0.id_str());
        EXPECT_STREQ(info->name(), b0.name());
        EXPECT_EQ(info->type(), b0.type());
        
        Er::Property propz;
        EXPECT_NE(prop2, propz);
    }

    {
        TestProps::Int64Prop b0(0);
        Er::Property prop0(b0);
        EXPECT_EQ(prop0.id, TestProps::Int64Prop::Id::value);
        EXPECT_EQ(prop0.type(), Er::PropertyType::Int64);
        EXPECT_EQ(Er::get<int64_t>(prop0.value), 0);

        Er::Property prop2(TestProps::Int64Prop(-2));
        EXPECT_EQ(prop2.id, TestProps::Int64Prop::Id::value);
        EXPECT_EQ(prop2.type(), Er::PropertyType::Int64);
        EXPECT_EQ(Er::get<int64_t>(prop2.value), -2);

        auto info = Er::lookupProperty("Test", prop2.id);
        ASSERT_TRUE(info);
        EXPECT_EQ(info->id(), TestProps::Int64Prop::Id::value);
        EXPECT_STREQ(info->id_str(), b0.id_str());
        EXPECT_STREQ(info->name(), b0.name());
        EXPECT_EQ(info->type(), b0.type());
    }

    {
        TestProps::UInt64Prop b0(0);
        Er::Property prop0(b0);
        EXPECT_EQ(prop0.id, TestProps::UInt64Prop::Id::value);
        EXPECT_EQ(prop0.type(), Er::PropertyType::UInt64);
        EXPECT_EQ(Er::get<uint64_t>(prop0.value), 0);

        Er::Property prop2(TestProps::UInt64Prop(0x8000100020003000ULL));
        EXPECT_EQ(prop2.id, TestProps::UInt64Prop::Id::value);
        EXPECT_EQ(prop2.type(), Er::PropertyType::UInt64);
        EXPECT_EQ(Er::get<uint64_t>(prop2.value), 0x8000100020003000ULL);

        auto info = Er::lookupProperty("Test", prop2.id);
        ASSERT_TRUE(info);
        EXPECT_EQ(info->id(), TestProps::UInt64Prop::Id::value);
        EXPECT_STREQ(info->id_str(), b0.id_str());
        EXPECT_STREQ(info->name(), b0.name());
        EXPECT_EQ(info->type(), b0.type());
    }

    {
        TestProps::DoubleProp b0(0);
        Er::Property prop0(b0);
        EXPECT_EQ(prop0.id, TestProps::DoubleProp::Id::value);
        EXPECT_EQ(prop0.type(), Er::PropertyType::Double);
        EXPECT_EQ(Er::get<double>(prop0.value), 0);

        Er::Property prop2(TestProps::DoubleProp(-1.03));
        EXPECT_EQ(prop2.id, TestProps::DoubleProp::Id::value);
        EXPECT_EQ(prop2.type(), Er::PropertyType::Double);
        EXPECT_EQ(Er::get<double>(prop2.value), -1.03);

        auto info = Er::lookupProperty("Test", prop2.id);
        ASSERT_TRUE(info);
        EXPECT_EQ(info->id(), TestProps::DoubleProp::Id::value);
        EXPECT_STREQ(info->id_str(), b0.id_str());
        EXPECT_STREQ(info->name(), b0.name());
        EXPECT_EQ(info->type(), b0.type());
    }

    {
        TestProps::StringProp b0("aaa.a");
        Er::Property prop0(b0);
        EXPECT_EQ(prop0.id, TestProps::StringProp::Id::value);
        EXPECT_EQ(prop0.type(), Er::PropertyType::String);
        EXPECT_STREQ(Er::get<std::string>(prop0.value).c_str(), "aaa.a");

        Er::Property prop2(TestProps::StringProp("b.bbb"));
        EXPECT_EQ(prop2.id, TestProps::StringProp::Id::value);
        EXPECT_EQ(prop2.type(), Er::PropertyType::String);
        EXPECT_STREQ(Er::get<std::string>(prop2.value).c_str(), "b.bbb");

        auto info = Er::lookupProperty("Test", prop2.id);
        ASSERT_TRUE(info);
        EXPECT_EQ(info->id(), TestProps::StringProp::Id::value);
        EXPECT_STREQ(info->id_str(), b0.id_str());
        EXPECT_STREQ(info->name(), b0.name());
        EXPECT_EQ(info->type(), b0.type());
    }

    {
        TestProps::BinaryProp b0(Er::Binary("aaa.a"));
        Er::Property prop0(b0);
        EXPECT_EQ(prop0.id, TestProps::BinaryProp::Id::value);
        EXPECT_EQ(prop0.type(), Er::PropertyType::Binary);
        EXPECT_STREQ(Er::get<Er::Binary>(prop0.value).bytes().c_str(), "aaa.a");

        Er::Property prop2(TestProps::BinaryProp(Er::Binary("b.bbb")));
        EXPECT_EQ(prop2.id, TestProps::BinaryProp::Id::value);
        EXPECT_EQ(prop2.type(), Er::PropertyType::Binary);
        EXPECT_STREQ(Er::get<Er::Binary>(prop2.value).bytes().c_str(), "b.bbb");

        auto info = Er::lookupProperty("Test", prop2.id);
        ASSERT_TRUE(info);
        EXPECT_EQ(info->id(), TestProps::BinaryProp::Id::value);
        EXPECT_STREQ(info->id_str(), b0.id_str());
        EXPECT_STREQ(info->name(), b0.name());
        EXPECT_EQ(info->type(), b0.type());
    }
}

TEST(Er_Property, constructFromRawValue)
{
    {
        Er::Property prop(TestProps::BoolProp::Id::value, Er::True);
        EXPECT_EQ(prop.id, TestProps::BoolProp::Id::value);
        EXPECT_EQ(prop.type(), Er::PropertyType::Bool);
        EXPECT_EQ(Er::get<Er::Bool>(prop.value), Er::True);
    }

    {
        Er::Property prop(TestProps::Int32Prop::Id::value, int32_t(-1));
        EXPECT_EQ(prop.id, TestProps::Int32Prop::Id::value);
        EXPECT_EQ(prop.type(), Er::PropertyType::Int32);
        EXPECT_EQ(Er::get<int32_t>(prop.value), -1);
    }

    {
        Er::Property prop(TestProps::UInt32Prop::Id::value, uint32_t(2));
        EXPECT_EQ(prop.id, TestProps::UInt32Prop::Id::value);
        EXPECT_EQ(prop.type(), Er::PropertyType::UInt32);
        EXPECT_EQ(Er::get<uint32_t>(prop.value), 2);
    }

    {
        Er::Property prop(TestProps::Int64Prop::Id::value, int64_t(-3));
        EXPECT_EQ(prop.id, TestProps::Int64Prop::Id::value);
        EXPECT_EQ(prop.type(), Er::PropertyType::Int64);
        EXPECT_EQ(Er::get<int64_t>(prop.value), -3);
    }

    {
        Er::Property prop(TestProps::UInt64Prop::Id::value, uint64_t(0x8000700060005000ULL));
        EXPECT_EQ(prop.id, TestProps::UInt64Prop::Id::value);
        EXPECT_EQ(prop.type(), Er::PropertyType::UInt64);
        EXPECT_EQ(Er::get<uint64_t>(prop.value), 0x8000700060005000ULL);
    }

    {
        Er::Property prop(TestProps::DoubleProp::Id::value, -0.12);
        EXPECT_EQ(prop.id, TestProps::DoubleProp::Id::value);
        EXPECT_EQ(prop.type(), Er::PropertyType::Double);
        EXPECT_EQ(Er::get<double>(prop.value), -0.12);
    }

    {
        Er::Property prop(TestProps::StringProp::Id::value, std::string("test string"));
        EXPECT_EQ(prop.id, TestProps::StringProp::Id::value);
        EXPECT_EQ(prop.type(), Er::PropertyType::String);
        EXPECT_STREQ(Er::get<std::string>(prop.value).c_str(), "test string");
    }

    {
        Er::Property prop(TestProps::BinaryProp::Id::value, Er::Binary(std::string("test string")));
        EXPECT_EQ(prop.id, TestProps::BinaryProp::Id::value);
        EXPECT_EQ(prop.type(), Er::PropertyType::Binary);
        EXPECT_STREQ(Er::get<Er::Binary>(prop.value).bytes().c_str(), "test string");
    }
}

TEST(Er_Property, constructFromProperty)
{
    {
        Er::Property prop0;
        EXPECT_TRUE(prop0.empty());

        Er::Property prop1(TestProps::StringProp::Id::value, std::string("test string"));
        EXPECT_FALSE(prop1.empty());
        EXPECT_EQ(prop1.id, TestProps::StringProp::Id::value);
        EXPECT_EQ(prop1.type(), Er::PropertyType::String);
        EXPECT_STREQ(Er::get<std::string>(prop1.value).c_str(), "test string");

        Er::Property prop2(prop1);
        EXPECT_FALSE(prop1.empty());
        EXPECT_FALSE(prop2.empty());
        EXPECT_EQ(prop2.id, TestProps::StringProp::Id::value);
        EXPECT_EQ(prop2.type(), Er::PropertyType::String);
        EXPECT_STREQ(Er::get<std::string>(prop2.value).c_str(), "test string");

        Er::Property prop3(std::move(prop1));
        EXPECT_FALSE(prop3.empty());
        EXPECT_EQ(prop3.id, TestProps::StringProp::Id::value);
        EXPECT_EQ(prop3.type(), Er::PropertyType::String);
        EXPECT_STREQ(Er::get<std::string>(prop3.value).c_str(), "test string");
    }
}

TEST(Er_Property, formatting)
{
    // Bool
    {
        Er::Property prop1(TestProps::BoolProp{ Er::False });
        Er::Property prop2(TestProps::BoolProp{ Er::True });
        
        auto info1 = Er::lookupProperty(TestProps::Domain, prop1.id);
        ASSERT_TRUE(info1);
        auto info2 = Er::lookupProperty(TestProps::Domain, prop2.id);
        EXPECT_EQ(info2.get(), info1.get());
        
        EXPECT_STREQ(info1->to_string(prop1).c_str(), "False");
        EXPECT_STREQ(prop1.to_string().c_str(), "False");
        EXPECT_STREQ(info2->to_string(prop2).c_str(), "True");
        EXPECT_STREQ(prop2.to_string().c_str(), "True");
    }

    // Int32
    {
        Er::Property prop(TestProps::Int32Prop{ -12 });

        auto info = Er::lookupProperty(TestProps::Domain, prop.id);
        ASSERT_TRUE(info);

        EXPECT_STREQ(info->to_string(prop).c_str(), "-12");
        EXPECT_STREQ(prop.to_string().c_str(), "-12");
    }

    // UInt32
    {
        Er::Property prop(TestProps::UInt32Prop{ 345 });

        auto info = Er::lookupProperty(TestProps::Domain, prop.id);
        ASSERT_TRUE(info);

        EXPECT_STREQ(info->to_string(prop).c_str(), "345");
        EXPECT_STREQ(prop.to_string().c_str(), "345");
    }

    // Int64
    {
        Er::Property prop(TestProps::Int64Prop{ -9223372036854775807LL });

        auto info = Er::lookupProperty(TestProps::Domain, prop.id);
        ASSERT_TRUE(info);

        EXPECT_STREQ(info->to_string(prop).c_str(), "-9223372036854775807");
        EXPECT_STREQ(prop.to_string().c_str(), "-9223372036854775807");
    }

    // UInt64
    {
        Er::Property prop(TestProps::Int64Prop{ 223372036854775807LL });

        auto info = Er::lookupProperty(TestProps::Domain, prop.id);
        ASSERT_TRUE(info);

        EXPECT_STREQ(info->to_string(prop).c_str(), "223372036854775807");
        EXPECT_STREQ(prop.to_string().c_str(), "223372036854775807");
    }

    // Double
    {
        Er::Property prop(TestProps::DoubleProp{ -123.789065 });

        auto info = Er::lookupProperty(TestProps::Domain, prop.id);
        ASSERT_TRUE(info);

        EXPECT_STREQ(info->to_string(prop).c_str(), "-123.789");
        EXPECT_STREQ(prop.to_string().c_str(), "-123.789");
    }

    // String
    {
        Er::Property prop(TestProps::StringProp{ "123.789065" });

        auto info = Er::lookupProperty(TestProps::Domain, prop.id);
        ASSERT_TRUE(info);

        EXPECT_STREQ(info->to_string(prop).c_str(), "123.789065");
        EXPECT_STREQ(prop.to_string().c_str(), "123.789065");
    }

    // Binary
    {
        Er::Property prop(TestProps::BinaryProp{ Er::Binary("abcdefgh") });

        auto info = Er::lookupProperty(TestProps::Domain, prop.id);
        ASSERT_TRUE(info);
        auto s = info->to_string(prop);
        EXPECT_STREQ(info->to_string(prop).c_str(), "61 62 63 64 65 66 67 68");
        EXPECT_STREQ(prop.to_string().c_str(), "61 62 63 64 65 66 67 68");
    }

    // empty vector
    {
        Er::Property prop(TestProps::BoolsProp{ std::vector<Er::Bool>{ } });

        auto info = Er::lookupProperty(TestProps::Domain, prop.id);
        ASSERT_TRUE(info);

        EXPECT_STREQ(info->to_string(prop).c_str(), "[]");
        EXPECT_STREQ(prop.to_string().c_str(), "[]");
    }

    // Bools
    {
        Er::Property prop(TestProps::BoolsProp{ std::vector<Er::Bool>{ Er::True, Er::False, Er::True } });
        
        auto info = Er::lookupProperty(TestProps::Domain, prop.id);
        ASSERT_TRUE(info);

        EXPECT_STREQ(info->to_string(prop).c_str(), "[ True, False, True ]");
        EXPECT_STREQ(prop.to_string().c_str(), "[ True, False, True ]");
    }

    // Int32s
    {
        Er::Property prop(TestProps::Int32sProp{ std::vector<int32_t>{ -1, 2, -3 } });

        auto info = Er::lookupProperty(TestProps::Domain, prop.id);
        ASSERT_TRUE(info);

        EXPECT_STREQ(info->to_string(prop).c_str(), "[ -1, 2, -3 ]");
        EXPECT_STREQ(prop.to_string().c_str(), "[ -1, 2, -3 ]");
    }

    // UInt32s
    {
        Er::Property prop(TestProps::UInt32sProp{ std::vector<uint32_t>{ 1, 2, 3 } });

        auto info = Er::lookupProperty(TestProps::Domain, prop.id);
        ASSERT_TRUE(info);

        EXPECT_STREQ(info->to_string(prop).c_str(), "[ 1, 2, 3 ]");
        EXPECT_STREQ(prop.to_string().c_str(), "[ 1, 2, 3 ]");
    }

    // Int64s
    {
        Er::Property prop(TestProps::Int64sProp{ std::vector<int64_t>{ -1, 2, -3 } });

        auto info = Er::lookupProperty(TestProps::Domain, prop.id);
        ASSERT_TRUE(info);

        EXPECT_STREQ(info->to_string(prop).c_str(), "[ -1, 2, -3 ]");
        EXPECT_STREQ(prop.to_string().c_str(), "[ -1, 2, -3 ]");
    }

    // UInt64s
    {
        Er::Property prop(TestProps::UInt64sProp{ std::vector<uint64_t>{ 1, 2, 3 } });

        auto info = Er::lookupProperty(TestProps::Domain, prop.id);
        ASSERT_TRUE(info);

        EXPECT_STREQ(info->to_string(prop).c_str(), "[ 1, 2, 3 ]");
        EXPECT_STREQ(prop.to_string().c_str(), "[ 1, 2, 3 ]");
    }

    // Doubles
    {
        Er::Property prop(TestProps::DoublesProp{ std::vector<double>{ -1.203034, 2.233223, -3.32333 } });

        auto info = Er::lookupProperty(TestProps::Domain, prop.id);
        ASSERT_TRUE(info);

        EXPECT_STREQ(info->to_string(prop).c_str(), "[ -1.203, 2.233, -3.323 ]");
        EXPECT_STREQ(prop.to_string().c_str(), "[ -1.203, 2.233, -3.323 ]");
    }

    // Strings
    {
        Er::Property prop(TestProps::StringsProp{ std::vector<std::string>{ "aaa", "bbb", "ccc" } });

        auto info = Er::lookupProperty(TestProps::Domain, prop.id);
        ASSERT_TRUE(info);

        EXPECT_STREQ(info->to_string(prop).c_str(), "[ \"aaa\", \"bbb\", \"ccc\" ]");
        EXPECT_STREQ(prop.to_string().c_str(), "[ \"aaa\", \"bbb\", \"ccc\" ]");
    }
}