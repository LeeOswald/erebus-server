#include "common.hpp"

#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>


TEST(Er_Property, registration)
{
    using SomeProp = Er::PropertyValue<bool, ER_PROPID("some_test_prop"), "SomeBool", Er::PropertyComparator<bool>, Er::PropertyFormatter<bool>>;
    Er::registerProperty(std::make_shared<Er::PropertyInfoWrapper<SomeProp>>(), g_log);
    
    auto somePropById = Er::lookupProperty(SomeProp::Id::value);
    ASSERT_TRUE(somePropById);
    auto somePropByStrId = Er::lookupProperty(SomeProp::id_str);
    ASSERT_TRUE(somePropByStrId);
    EXPECT_EQ(somePropById.get(), somePropByStrId.get());
    EXPECT_EQ(somePropById->type(), Er::PropertyType::Bool);
    EXPECT_EQ(somePropById->id(), SomeProp::Id::value);
    EXPECT_STREQ(somePropById->id_str(), "some_test_prop");
    EXPECT_STREQ(somePropById->name(), "SomeBool");

    Er::unregisterProperty(Er::lookupProperty(SomeProp::Id::value), g_log);
    auto no = Er::lookupProperty(SomeProp::Id::value);
    EXPECT_FALSE(no);
}

TEST(Er_Property, constructFromPropertyValue)
{
    {
        TestProps::BoolProp bf(false);
        Er::Property propf(bf);
        EXPECT_EQ(propf.id, TestProps::BoolProp::Id::value);
        EXPECT_EQ(propf.type(), Er::PropertyType::Bool);
        EXPECT_FALSE(std::get<bool>(propf.value));

        Er::Property propt(TestProps::BoolProp(true));
        EXPECT_EQ(propt.id, TestProps::BoolProp::Id::value);
        EXPECT_EQ(propt.type(), Er::PropertyType::Bool);
        EXPECT_TRUE(std::get<bool>(propt.value));

        auto info = Er::lookupProperty(propt.id);
        ASSERT_TRUE(info);
        EXPECT_FALSE(info->equal(propf, propt));
        EXPECT_TRUE(info->equal(Er::Property(TestProps::BoolProp(true)), Er::Property(TestProps::BoolProp(true))));
        EXPECT_FALSE(info->equal(Er::Property(TestProps::BoolProp(true)), Er::Property(TestProps::Int32Prop(1))));
        EXPECT_EQ(info->id(), TestProps::BoolProp::Id::value);
        EXPECT_STREQ(info->id_str(), bf.id_str);
        EXPECT_STREQ(info->name(), bf.name);
        EXPECT_EQ(info->type(), bf.type);
        EXPECT_EQ(info->size(propt), sizeof(bool));
        ASSERT_TRUE(info->data(propt));
        EXPECT_EQ(*static_cast<const bool*>(info->data(propt)), true);
        EXPECT_EQ(*static_cast<const bool*>(info->data(propf)), false);
        
        Er::Property propz;
        EXPECT_EQ(info->size(propz), 0);
        EXPECT_FALSE(info->data(propz));
        EXPECT_FALSE(info->equal(propt, propz));
    }

    {
        TestProps::Int32Prop b0(0);
        Er::Property prop0(b0);
        EXPECT_EQ(prop0.id, TestProps::Int32Prop::Id::value);
        EXPECT_EQ(prop0.type(), Er::PropertyType::Int32);
        EXPECT_EQ(std::get<int32_t>(prop0.value), 0);

        Er::Property prop2(TestProps::Int32Prop(-2));
        EXPECT_EQ(prop2.id, TestProps::Int32Prop::Id::value);
        EXPECT_EQ(prop2.type(), Er::PropertyType::Int32);
        EXPECT_EQ(std::get<int32_t>(prop2.value), -2);

        auto info = Er::lookupProperty(prop2.id);
        ASSERT_TRUE(info);
        EXPECT_FALSE(info->equal(prop0, prop2));
        EXPECT_TRUE(info->equal(Er::Property(TestProps::Int32Prop(3)), Er::Property(TestProps::Int32Prop(3))));
        EXPECT_FALSE(info->equal(Er::Property(TestProps::Int32Prop(3)), Er::Property(TestProps::UInt32Prop(3))));
        EXPECT_EQ(info->id(), TestProps::Int32Prop::Id::value);
        EXPECT_STREQ(info->id_str(), b0.id_str);
        EXPECT_STREQ(info->name(), b0.name);
        EXPECT_EQ(info->type(), b0.type);
        EXPECT_EQ(info->size(prop0), sizeof(int32_t));
        ASSERT_TRUE(info->data(prop0));
        EXPECT_EQ(*static_cast<const int32_t*>(info->data(prop0)), 0);
        EXPECT_EQ(*static_cast<const int32_t*>(info->data(prop2)), -2);

        Er::Property propz;
        EXPECT_EQ(info->size(propz), 0);
        EXPECT_FALSE(info->data(propz));
        EXPECT_FALSE(info->equal(prop2, propz));
    }

    {
        TestProps::UInt32Prop b0(0);
        Er::Property prop0(b0);
        EXPECT_EQ(prop0.id, TestProps::UInt32Prop::Id::value);
        EXPECT_EQ(prop0.type(), Er::PropertyType::UInt32);
        EXPECT_EQ(std::get<uint32_t>(prop0.value), 0);

        Er::Property prop2(TestProps::UInt32Prop(2));
        EXPECT_EQ(prop2.id, TestProps::UInt32Prop::Id::value);
        EXPECT_EQ(prop2.type(), Er::PropertyType::UInt32);
        EXPECT_EQ(std::get<uint32_t>(prop2.value), 2);

        auto info = Er::lookupProperty(prop2.id);
        ASSERT_TRUE(info);
        EXPECT_FALSE(info->equal(prop0, prop2));
        EXPECT_TRUE(info->equal(Er::Property(TestProps::UInt32Prop(5)), Er::Property(TestProps::UInt32Prop(5))));
        EXPECT_FALSE(info->equal(Er::Property(TestProps::UInt32Prop(5)), Er::Property(TestProps::Int32Prop(5))));
        EXPECT_EQ(info->id(), TestProps::UInt32Prop::Id::value);
        EXPECT_STREQ(info->id_str(), b0.id_str);
        EXPECT_STREQ(info->name(), b0.name);
        EXPECT_EQ(info->type(), b0.type);
        EXPECT_EQ(info->size(prop0), sizeof(uint32_t));
        ASSERT_TRUE(info->data(prop0));
        EXPECT_EQ(*static_cast<const uint32_t*>(info->data(prop0)), 0);
        EXPECT_EQ(*static_cast<const uint32_t*>(info->data(prop2)), 2);

        Er::Property propz;
        EXPECT_EQ(info->size(propz), 0);
        EXPECT_FALSE(info->data(propz));
        EXPECT_FALSE(info->equal(prop2, propz));
    }

    {
        TestProps::Int64Prop b0(0);
        Er::Property prop0(b0);
        EXPECT_EQ(prop0.id, TestProps::Int64Prop::Id::value);
        EXPECT_EQ(prop0.type(), Er::PropertyType::Int64);
        EXPECT_EQ(std::get<int64_t>(prop0.value), 0);

        Er::Property prop2(TestProps::Int64Prop(-2));
        EXPECT_EQ(prop2.id, TestProps::Int64Prop::Id::value);
        EXPECT_EQ(prop2.type(), Er::PropertyType::Int64);
        EXPECT_EQ(std::get<int64_t>(prop2.value), -2);

        auto info = Er::lookupProperty(prop2.id);
        ASSERT_TRUE(info);
        EXPECT_FALSE(info->equal(prop0, prop2));
        EXPECT_TRUE(info->equal(Er::Property(TestProps::Int64Prop(3)), Er::Property(TestProps::Int64Prop(3))));
        EXPECT_FALSE(info->equal(Er::Property(TestProps::Int64Prop(3)), Er::Property(TestProps::UInt64Prop(3))));
        EXPECT_EQ(info->id(), TestProps::Int64Prop::Id::value);
        EXPECT_STREQ(info->id_str(), b0.id_str);
        EXPECT_STREQ(info->name(), b0.name);
        EXPECT_EQ(info->type(), b0.type);
        EXPECT_EQ(info->size(prop0), sizeof(int64_t));
        ASSERT_TRUE(info->data(prop0));
        EXPECT_EQ(*static_cast<const int64_t*>(info->data(prop0)), 0);
        EXPECT_EQ(*static_cast<const int64_t*>(info->data(prop2)), -2);

        Er::Property propz;
        EXPECT_EQ(info->size(propz), 0);
        EXPECT_FALSE(info->data(propz));
        EXPECT_FALSE(info->equal(prop2, propz));
    }

    {
        TestProps::UInt64Prop b0(0);
        Er::Property prop0(b0);
        EXPECT_EQ(prop0.id, TestProps::UInt64Prop::Id::value);
        EXPECT_EQ(prop0.type(), Er::PropertyType::UInt64);
        EXPECT_EQ(std::get<uint64_t>(prop0.value), 0);

        Er::Property prop2(TestProps::UInt64Prop(0x8000100020003000ULL));
        EXPECT_EQ(prop2.id, TestProps::UInt64Prop::Id::value);
        EXPECT_EQ(prop2.type(), Er::PropertyType::UInt64);
        EXPECT_EQ(std::get<uint64_t>(prop2.value), 0x8000100020003000ULL);

        auto info = Er::lookupProperty(prop2.id);
        ASSERT_TRUE(info);
        EXPECT_FALSE(info->equal(prop0, prop2));
        EXPECT_TRUE(info->equal(Er::Property(TestProps::UInt64Prop(3)), Er::Property(TestProps::UInt64Prop(3))));
        EXPECT_FALSE(info->equal(Er::Property(TestProps::UInt64Prop(3)), Er::Property(TestProps::Int64Prop(3))));
        EXPECT_EQ(info->id(), TestProps::UInt64Prop::Id::value);
        EXPECT_STREQ(info->id_str(), b0.id_str);
        EXPECT_STREQ(info->name(), b0.name);
        EXPECT_EQ(info->type(), b0.type);
        EXPECT_EQ(info->size(prop0), sizeof(uint64_t));
        ASSERT_TRUE(info->data(prop0));
        EXPECT_EQ(*static_cast<const uint64_t*>(info->data(prop0)), 0);
        EXPECT_EQ(*static_cast<const uint64_t*>(info->data(prop2)), 0x8000100020003000ULL);

        Er::Property propz;
        EXPECT_EQ(info->size(propz), 0);
        EXPECT_FALSE(info->data(propz));
        EXPECT_FALSE(info->equal(prop2, propz));
    }

    {
        TestProps::DoubleProp b0(0);
        Er::Property prop0(b0);
        EXPECT_EQ(prop0.id, TestProps::DoubleProp::Id::value);
        EXPECT_EQ(prop0.type(), Er::PropertyType::Double);
        EXPECT_EQ(std::get<double>(prop0.value), 0);

        Er::Property prop2(TestProps::DoubleProp(-1.03));
        EXPECT_EQ(prop2.id, TestProps::DoubleProp::Id::value);
        EXPECT_EQ(prop2.type(), Er::PropertyType::Double);
        EXPECT_EQ(std::get<double>(prop2.value), -1.03);

        auto info = Er::lookupProperty(prop2.id);
        ASSERT_TRUE(info);
        EXPECT_FALSE(info->equal(prop0, prop2));
        EXPECT_TRUE(info->equal(Er::Property(TestProps::DoubleProp(3)), Er::Property(TestProps::DoubleProp(3))));
        EXPECT_FALSE(info->equal(Er::Property(TestProps::DoubleProp(3)), Er::Property(TestProps::Int64Prop(3))));
        EXPECT_EQ(info->id(), TestProps::DoubleProp::Id::value);
        EXPECT_STREQ(info->id_str(), b0.id_str);
        EXPECT_STREQ(info->name(), b0.name);
        EXPECT_EQ(info->type(), b0.type);
        EXPECT_EQ(info->size(prop0), sizeof(double));
        ASSERT_TRUE(info->data(prop0));
        EXPECT_EQ(*static_cast<const double*>(info->data(prop0)), 0);
        EXPECT_EQ(*static_cast<const double*>(info->data(prop2)), -1.03);

        Er::Property propz;
        EXPECT_EQ(info->size(propz), 0);
        EXPECT_FALSE(info->data(propz));
        EXPECT_FALSE(info->equal(prop2, propz));
    }

    {
        TestProps::StringProp b0("aaa.a");
        Er::Property prop0(b0);
        EXPECT_EQ(prop0.id, TestProps::StringProp::Id::value);
        EXPECT_EQ(prop0.type(), Er::PropertyType::String);
        EXPECT_STREQ(std::get<std::string>(prop0.value).c_str(), "aaa.a");

        Er::Property prop2(TestProps::StringProp("b.bbb"));
        EXPECT_EQ(prop2.id, TestProps::StringProp::Id::value);
        EXPECT_EQ(prop2.type(), Er::PropertyType::String);
        EXPECT_STREQ(std::get<std::string>(prop2.value).c_str(), "b.bbb");

        auto info = Er::lookupProperty(prop2.id);
        ASSERT_TRUE(info);
        EXPECT_FALSE(info->equal(prop0, prop2));
        EXPECT_TRUE(info->equal(Er::Property(TestProps::StringProp("zzzz")), Er::Property(TestProps::StringProp("zzzz"))));
        EXPECT_FALSE(info->equal(Er::Property(TestProps::StringProp("zzzz")), Er::Property(TestProps::StringProp("zzzy"))));
        EXPECT_EQ(info->id(), TestProps::StringProp::Id::value);
        EXPECT_STREQ(info->id_str(), b0.id_str);
        EXPECT_STREQ(info->name(), b0.name);
        EXPECT_EQ(info->type(), b0.type);
        EXPECT_EQ(info->size(prop0), 5);
        ASSERT_TRUE(info->data(prop0));
        EXPECT_STREQ(static_cast<const char*>(info->data(prop0)), "aaa.a");
        EXPECT_STREQ(static_cast<const char*>(info->data(prop2)), "b.bbb");

        Er::Property propz;
        EXPECT_EQ(info->size(propz), 0);
        EXPECT_FALSE(info->data(propz));
        EXPECT_FALSE(info->equal(prop2, propz));
    }

    {
        TestProps::BytesProp b0(Er::Bytes("aaa.a"));
        Er::Property prop0(b0);
        EXPECT_EQ(prop0.id, TestProps::BytesProp::Id::value);
        EXPECT_EQ(prop0.type(), Er::PropertyType::Bytes);
        EXPECT_STREQ(std::get<Er::Bytes>(prop0.value).bytes().c_str(), "aaa.a");

        Er::Property prop2(TestProps::BytesProp(Er::Bytes("b.bbb")));
        EXPECT_EQ(prop2.id, TestProps::BytesProp::Id::value);
        EXPECT_EQ(prop2.type(), Er::PropertyType::Bytes);
        EXPECT_STREQ(std::get<Er::Bytes>(prop2.value).bytes().c_str(), "b.bbb");

        auto info = Er::lookupProperty(prop2.id);
        ASSERT_TRUE(info);
        EXPECT_FALSE(info->equal(prop0, prop2));
        EXPECT_TRUE(info->equal(Er::Property(TestProps::BytesProp(Er::Bytes("zzzz"))), Er::Property(TestProps::BytesProp(Er::Bytes("zzzz")))));
        EXPECT_FALSE(info->equal(Er::Property(TestProps::BytesProp(Er::Bytes("zzzz"))), Er::Property(TestProps::BytesProp(Er::Bytes("zzzy")))));
        EXPECT_EQ(info->id(), TestProps::BytesProp::Id::value);
        EXPECT_STREQ(info->id_str(), b0.id_str);
        EXPECT_STREQ(info->name(), b0.name);
        EXPECT_EQ(info->type(), b0.type);
        EXPECT_EQ(info->size(prop0), 5);
        ASSERT_TRUE(info->data(prop0));
        EXPECT_STREQ(static_cast<const char*>(info->data(prop0)), "aaa.a");
        EXPECT_STREQ(static_cast<const char*>(info->data(prop2)), "b.bbb");

        Er::Property propz;
        EXPECT_EQ(info->size(propz), 0);
        EXPECT_FALSE(info->data(propz));
        EXPECT_FALSE(info->equal(prop2, propz));
    }
}

TEST(Er_Property, constructFromRawValue)
{
    {
        Er::Property prop(TestProps::BoolProp::Id::value, true);
        EXPECT_EQ(prop.id, TestProps::BoolProp::Id::value);
        EXPECT_EQ(prop.type(), Er::PropertyType::Bool);
        EXPECT_EQ(std::get<bool>(prop.value), true);
    }

    {
        Er::Property prop(TestProps::Int32Prop::Id::value, int32_t(-1));
        EXPECT_EQ(prop.id, TestProps::Int32Prop::Id::value);
        EXPECT_EQ(prop.type(), Er::PropertyType::Int32);
        EXPECT_EQ(std::get<int32_t>(prop.value), -1);
    }

    {
        Er::Property prop(TestProps::UInt32Prop::Id::value, uint32_t(2));
        EXPECT_EQ(prop.id, TestProps::UInt32Prop::Id::value);
        EXPECT_EQ(prop.type(), Er::PropertyType::UInt32);
        EXPECT_EQ(std::get<uint32_t>(prop.value), 2);
    }

    {
        Er::Property prop(TestProps::Int64Prop::Id::value, int64_t(-3));
        EXPECT_EQ(prop.id, TestProps::Int64Prop::Id::value);
        EXPECT_EQ(prop.type(), Er::PropertyType::Int64);
        EXPECT_EQ(std::get<int64_t>(prop.value), -3);
    }

    {
        Er::Property prop(TestProps::UInt64Prop::Id::value, uint64_t(0x8000700060005000ULL));
        EXPECT_EQ(prop.id, TestProps::UInt64Prop::Id::value);
        EXPECT_EQ(prop.type(), Er::PropertyType::UInt64);
        EXPECT_EQ(std::get<uint64_t>(prop.value), 0x8000700060005000ULL);
    }

    {
        Er::Property prop(TestProps::DoubleProp::Id::value, -0.12);
        EXPECT_EQ(prop.id, TestProps::DoubleProp::Id::value);
        EXPECT_EQ(prop.type(), Er::PropertyType::Double);
        EXPECT_EQ(std::get<double>(prop.value), -0.12);
    }

    {
        Er::Property prop(TestProps::StringProp::Id::value, std::string("test string"));
        EXPECT_EQ(prop.id, TestProps::StringProp::Id::value);
        EXPECT_EQ(prop.type(), Er::PropertyType::String);
        EXPECT_STREQ(std::get<std::string>(prop.value).c_str(), "test string");
    }

    {
        Er::Property prop(TestProps::BytesProp::Id::value, Er::Bytes(std::string("test string")));
        EXPECT_EQ(prop.id, TestProps::BytesProp::Id::value);
        EXPECT_EQ(prop.type(), Er::PropertyType::Bytes);
        EXPECT_STREQ(std::get<Er::Bytes>(prop.value).bytes().c_str(), "test string");
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
        EXPECT_STREQ(std::get<std::string>(prop1.value).c_str(), "test string");

        Er::Property prop2(prop1);
        EXPECT_FALSE(prop1.empty());
        EXPECT_FALSE(prop2.empty());
        EXPECT_EQ(prop2.id, TestProps::StringProp::Id::value);
        EXPECT_EQ(prop2.type(), Er::PropertyType::String);
        EXPECT_STREQ(std::get<std::string>(prop2.value).c_str(), "test string");

        Er::Property prop3(std::move(prop1));
        EXPECT_TRUE(std::get<std::string>(prop1.value).empty());
        EXPECT_FALSE(prop3.empty());
        EXPECT_EQ(prop3.id, TestProps::StringProp::Id::value);
        EXPECT_EQ(prop3.type(), Er::PropertyType::String);
        EXPECT_STREQ(std::get<std::string>(prop3.value).c_str(), "test string");
    }
}