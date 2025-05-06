#include "common.hpp"

#include <erebus/rtl/error.hxx>


using namespace Er;


TEST(Error, category)
{
#if ER_WINDOWS
    {
        auto cat = lookupErrorCategory(Win32Error->name());
        ASSERT_TRUE(!!cat);
        EXPECT_EQ(cat, Win32Error);

        auto descr = cat->message(ERROR_ACCESS_DENIED);
        EXPECT_FALSE(descr.empty());
        ErLogDebug("ERROR_ACCESS_DENIED -> [{}]", descr);
    }
#endif

    {
        auto cat = lookupErrorCategory(PosixError->name());
        ASSERT_TRUE(!!cat);
        EXPECT_EQ(cat, PosixError);

        auto descr = cat->message(ENOENT);
        EXPECT_FALSE(descr.empty());
        ErLogDebug("ENOENT -> [{}]", descr);
    }
}

TEST(Error, construction)
{
    // empty
    {
        Error e;
        EXPECT_FALSE(e);
        EXPECT_EQ(e.code(), Error::Success);
        EXPECT_EQ(e.category(), nullptr);
        EXPECT_TRUE(e.properties().empty());
    }

    // code only
    {
        Error e(ENOENT, PosixError);
        EXPECT_TRUE(e);
        EXPECT_EQ(e.code(), ENOENT);
        EXPECT_EQ(e.category(), PosixError);
        EXPECT_TRUE(e.properties().empty());
    }

    // add props in the constructor
    {
        ErrorProperties::ErrorProperty<std::string, Property::Type::String, "String"> prop1("Test string 1");

        EXPECT_EQ(prop1.type(), Property::Type::String);
        EXPECT_EQ(prop1.nameStr(), std::string_view("String"));
        EXPECT_STREQ(prop1.getString()->c_str(), "Test string 1");

        ErrorProperties::ErrorProperty<std::string, Property::Type::String, "String"> prop2("Test string 2");

        EXPECT_EQ(prop2.type(), Property::Type::String);
        EXPECT_EQ(prop2.nameStr(), std::string_view("String"));
        EXPECT_STREQ(prop2.getString()->c_str(), "Test string 2");

        ErrorProperties::ErrorProperty<std::int32_t, Property::Type::Int32, "Int32"> prop3(-123);

        EXPECT_EQ(prop3.type(), Property::Type::Int32);
        EXPECT_EQ(prop3.nameStr(), std::string_view("Int32"));
        EXPECT_EQ(*prop3.getInt32(), -123);

        ErrorProperties::ErrorProperty<std::uint64_t, Property::Type::UInt64, "UInt64"> prop4(0x8000000010002000ULL);

        EXPECT_EQ(prop4.type(), Property::Type::UInt64);
        EXPECT_EQ(prop4.nameStr(), std::string_view("UInt64"));
        EXPECT_EQ(*prop4.getUInt64(), 0x8000000010002000ULL);

        Error e(ENOENT, PosixError, prop1, std::move(prop2), prop3, std::move(prop4));

        EXPECT_FALSE(prop1.empty());
        EXPECT_TRUE(prop2.empty()); // moved-from
        EXPECT_FALSE(prop3.empty());
        EXPECT_TRUE(prop4.empty()); // moved-from

        EXPECT_TRUE(e);
        EXPECT_EQ(e.code(), ENOENT);
        EXPECT_EQ(e.category(), PosixError);
        EXPECT_FALSE(e.properties().empty());
        ASSERT_EQ(e.properties().size(), 4);

        ASSERT_NE(e.properties()[0].getString(), nullptr);
        EXPECT_STREQ(e.properties()[0].getString()->c_str(), "Test string 1");
        EXPECT_EQ(e.properties()[0].nameStr(), std::string_view("String"));

        ASSERT_NE(e.properties()[1].getString(), nullptr);
        EXPECT_STREQ(e.properties()[1].getString()->c_str(), "Test string 2");
        EXPECT_EQ(e.properties()[1].nameStr(), std::string_view("String"));

        ASSERT_NE(e.properties()[2].getInt32(), nullptr);
        EXPECT_EQ(*e.properties()[2].getInt32(), -123);
        EXPECT_EQ(e.properties()[2].nameStr(), std::string_view("Int32"));

        ASSERT_NE(e.properties()[3].getUInt64(), nullptr);
        EXPECT_EQ(*e.properties()[3].getUInt64(), 0x8000000010002000ULL);
        EXPECT_EQ(e.properties()[3].nameStr(), std::string_view("UInt64"));
    }

    // add()
    {
        ErrorProperties::ErrorProperty<std::string, Property::Type::String, "String"> prop1("Test string 1");
        ErrorProperties::ErrorProperty<std::string, Property::Type::String, "String"> prop2("Test string 2");
        ErrorProperties::ErrorProperty<std::int32_t, Property::Type::Int32, "Int32"> prop3(-123);
        ErrorProperties::ErrorProperty<std::uint64_t, Property::Type::UInt64, "UInt64"> prop4(0x8000000010002000ULL);

        Error e(ENOENT, PosixError);
        e.add(prop1).add(std::move(prop2)).add(prop3).add(std::move(prop4));

        EXPECT_FALSE(prop1.empty());
        EXPECT_TRUE(prop2.empty()); // moved-from
        EXPECT_FALSE(prop3.empty());
        EXPECT_TRUE(prop4.empty()); // moved-from

        EXPECT_TRUE(e);
        EXPECT_EQ(e.code(), ENOENT);
        EXPECT_EQ(e.category(), PosixError);
        EXPECT_FALSE(e.properties().empty());
        ASSERT_EQ(e.properties().size(), 4);

        ASSERT_NE(e.properties()[0].getString(), nullptr);
        EXPECT_STREQ(e.properties()[0].getString()->c_str(), "Test string 1");
        EXPECT_EQ(e.properties()[0].nameStr(), std::string_view("String"));

        ASSERT_NE(e.properties()[1].getString(), nullptr);
        EXPECT_STREQ(e.properties()[1].getString()->c_str(), "Test string 2");
        EXPECT_EQ(e.properties()[1].nameStr(), std::string_view("String"));

        ASSERT_NE(e.properties()[2].getInt32(), nullptr);
        EXPECT_EQ(*e.properties()[2].getInt32(), -123);
        EXPECT_EQ(e.properties()[2].nameStr(), std::string_view("Int32"));

        ASSERT_NE(e.properties()[3].getUInt64(), nullptr);
        EXPECT_EQ(*e.properties()[3].getUInt64(), 0x8000000010002000ULL);
        EXPECT_EQ(e.properties()[3].nameStr(), std::string_view("UInt64"));
    }
}

TEST(Error, copymove)
{
    // copy-construct
    {
        Error e1(
            ENOENT,
            PosixError,
            ErrorProperties::ErrorProperty<std::string, Property::Type::String, "String">("Test string 1"),
            ErrorProperties::ErrorProperty<std::string, Property::Type::String, "String">("Test string 2"),
            ErrorProperties::ErrorProperty<std::int32_t, Property::Type::Int32, "Int32">(-123),
            ErrorProperties::ErrorProperty<std::uint64_t, Property::Type::UInt64, "UInt64">(0x8000000010002000ULL)
        );

        Error e2(e1);

        EXPECT_TRUE(e1);
        EXPECT_EQ(e1.code(), ENOENT);
        EXPECT_EQ(e1.category(), PosixError);
        EXPECT_FALSE(e1.properties().empty());
        ASSERT_EQ(e1.properties().size(), 4);

        ASSERT_NE(e1.properties()[0].getString(), nullptr);
        EXPECT_STREQ(e1.properties()[0].getString()->c_str(), "Test string 1");
        EXPECT_EQ(e1.properties()[0].nameStr(), std::string_view("String"));

        ASSERT_NE(e1.properties()[1].getString(), nullptr);
        EXPECT_STREQ(e1.properties()[1].getString()->c_str(), "Test string 2");
        EXPECT_EQ(e1.properties()[1].nameStr(), std::string_view("String"));

        ASSERT_NE(e1.properties()[2].getInt32(), nullptr);
        EXPECT_EQ(*e1.properties()[2].getInt32(), -123);
        EXPECT_EQ(e1.properties()[2].nameStr(), std::string_view("Int32"));

        ASSERT_NE(e1.properties()[3].getUInt64(), nullptr);
        EXPECT_EQ(*e1.properties()[3].getUInt64(), 0x8000000010002000ULL);
        EXPECT_EQ(e1.properties()[3].nameStr(), std::string_view("UInt64"));

        EXPECT_TRUE(e2);
        EXPECT_EQ(e2.code(), ENOENT);
        EXPECT_EQ(e2.category(), PosixError);
        EXPECT_FALSE(e2.properties().empty());
        ASSERT_EQ(e2.properties().size(), 4);

        ASSERT_NE(e2.properties()[0].getString(), nullptr);
        EXPECT_STREQ(e2.properties()[0].getString()->c_str(), "Test string 1");
        EXPECT_EQ(e2.properties()[0].nameStr(), std::string_view("String"));

        ASSERT_NE(e2.properties()[1].getString(), nullptr);
        EXPECT_STREQ(e2.properties()[1].getString()->c_str(), "Test string 2");
        EXPECT_EQ(e2.properties()[1].nameStr(), std::string_view("String"));

        ASSERT_NE(e2.properties()[2].getInt32(), nullptr);
        EXPECT_EQ(*e2.properties()[2].getInt32(), -123);
        EXPECT_EQ(e2.properties()[2].nameStr(), std::string_view("Int32"));

        ASSERT_NE(e2.properties()[3].getUInt64(), nullptr);
        EXPECT_EQ(*e2.properties()[3].getUInt64(), 0x8000000010002000ULL);
        EXPECT_EQ(e2.properties()[3].nameStr(), std::string_view("UInt64"));
    }

    // copy-assign
    {
        Error e1(
            ENOENT,
            PosixError,
            ErrorProperties::ErrorProperty<std::string, Property::Type::String, "String">("Test string 1"),
            ErrorProperties::ErrorProperty<std::string, Property::Type::String, "String">("Test string 2"),
            ErrorProperties::ErrorProperty<std::int32_t, Property::Type::Int32, "Int32">(-123),
            ErrorProperties::ErrorProperty<std::uint64_t, Property::Type::UInt64, "UInt64">(0x8000000010002000ULL)
        );

        Error e2(
            EINVAL,
            PosixError,
            ErrorProperties::ErrorProperty<double, Property::Type::Double, "Double">(-0.9)
        );

        e2 = e1;

        EXPECT_TRUE(e1);
        EXPECT_EQ(e1.code(), ENOENT);
        EXPECT_EQ(e1.category(), PosixError);
        EXPECT_FALSE(e1.properties().empty());
        ASSERT_EQ(e1.properties().size(), 4);

        ASSERT_NE(e1.properties()[0].getString(), nullptr);
        EXPECT_STREQ(e1.properties()[0].getString()->c_str(), "Test string 1");
        EXPECT_EQ(e1.properties()[0].nameStr(), std::string_view("String"));

        ASSERT_NE(e1.properties()[1].getString(), nullptr);
        EXPECT_STREQ(e1.properties()[1].getString()->c_str(), "Test string 2");
        EXPECT_EQ(e1.properties()[1].nameStr(), std::string_view("String"));

        ASSERT_NE(e1.properties()[2].getInt32(), nullptr);
        EXPECT_EQ(*e1.properties()[2].getInt32(), -123);
        EXPECT_EQ(e1.properties()[2].nameStr(), std::string_view("Int32"));

        ASSERT_NE(e1.properties()[3].getUInt64(), nullptr);
        EXPECT_EQ(*e1.properties()[3].getUInt64(), 0x8000000010002000ULL);
        EXPECT_EQ(e1.properties()[3].nameStr(), std::string_view("UInt64"));

        EXPECT_TRUE(e2);
        EXPECT_EQ(e2.code(), ENOENT);
        EXPECT_EQ(e2.category(), PosixError);
        EXPECT_FALSE(e2.properties().empty());
        ASSERT_EQ(e2.properties().size(), 4);

        ASSERT_NE(e2.properties()[0].getString(), nullptr);
        EXPECT_STREQ(e2.properties()[0].getString()->c_str(), "Test string 1");
        EXPECT_EQ(e2.properties()[0].nameStr(), std::string_view("String"));

        ASSERT_NE(e2.properties()[1].getString(), nullptr);
        EXPECT_STREQ(e2.properties()[1].getString()->c_str(), "Test string 2");
        EXPECT_EQ(e2.properties()[1].nameStr(), std::string_view("String"));

        ASSERT_NE(e2.properties()[2].getInt32(), nullptr);
        EXPECT_EQ(*e2.properties()[2].getInt32(), -123);
        EXPECT_EQ(e2.properties()[2].nameStr(), std::string_view("Int32"));

        ASSERT_NE(e2.properties()[3].getUInt64(), nullptr);
        EXPECT_EQ(*e2.properties()[3].getUInt64(), 0x8000000010002000ULL);
        EXPECT_EQ(e2.properties()[3].nameStr(), std::string_view("UInt64"));
    }

    // move-construct
    {
        Error e1(
            ENOENT,
            PosixError,
            ErrorProperties::ErrorProperty<std::string, Property::Type::String, "String">("Test string 1"),
            ErrorProperties::ErrorProperty<std::string, Property::Type::String, "String">("Test string 2"),
            ErrorProperties::ErrorProperty<std::int32_t, Property::Type::Int32, "Int32">(-123),
            ErrorProperties::ErrorProperty<std::uint64_t, Property::Type::UInt64, "UInt64">(0x8000000010002000ULL)
        );

        Error e2(std::move(e1));

        EXPECT_TRUE(e1.properties().empty());

        EXPECT_TRUE(e2);
        EXPECT_EQ(e2.code(), ENOENT);
        EXPECT_EQ(e2.category(), PosixError);
        EXPECT_FALSE(e2.properties().empty());
        ASSERT_EQ(e2.properties().size(), 4);

        ASSERT_NE(e2.properties()[0].getString(), nullptr);
        EXPECT_STREQ(e2.properties()[0].getString()->c_str(), "Test string 1");
        EXPECT_EQ(e2.properties()[0].nameStr(), std::string_view("String"));

        ASSERT_NE(e2.properties()[1].getString(), nullptr);
        EXPECT_STREQ(e2.properties()[1].getString()->c_str(), "Test string 2");
        EXPECT_EQ(e2.properties()[1].nameStr(), std::string_view("String"));

        ASSERT_NE(e2.properties()[2].getInt32(), nullptr);
        EXPECT_EQ(*e2.properties()[2].getInt32(), -123);
        EXPECT_EQ(e2.properties()[2].nameStr(), std::string_view("Int32"));

        ASSERT_NE(e2.properties()[3].getUInt64(), nullptr);
        EXPECT_EQ(*e2.properties()[3].getUInt64(), 0x8000000010002000ULL);
        EXPECT_EQ(e2.properties()[3].nameStr(), std::string_view("UInt64"));
    }

    // move-assign
    {
        Error e1(
            ENOENT,
            PosixError,
            ErrorProperties::ErrorProperty<std::string, Property::Type::String, "String">("Test string 1"),
            ErrorProperties::ErrorProperty<std::string, Property::Type::String, "String">("Test string 2"),
            ErrorProperties::ErrorProperty<std::int32_t, Property::Type::Int32, "Int32">(-123),
            ErrorProperties::ErrorProperty<std::uint64_t, Property::Type::UInt64, "UInt64">(0x8000000010002000ULL)
        );

        Error e2(
            EINVAL,
            PosixError,
            ErrorProperties::ErrorProperty<double, Property::Type::Double, "Double">(-0.9)
        );
        
        e2 = std::move(e1);

        EXPECT_TRUE(e1.properties().empty());

        EXPECT_TRUE(e2);
        EXPECT_EQ(e2.code(), ENOENT);
        EXPECT_EQ(e2.category(), PosixError);
        EXPECT_FALSE(e2.properties().empty());
        ASSERT_EQ(e2.properties().size(), 4);

        ASSERT_NE(e2.properties()[0].getString(), nullptr);
        EXPECT_STREQ(e2.properties()[0].getString()->c_str(), "Test string 1");
        EXPECT_EQ(e2.properties()[0].nameStr(), std::string_view("String"));

        ASSERT_NE(e2.properties()[1].getString(), nullptr);
        EXPECT_STREQ(e2.properties()[1].getString()->c_str(), "Test string 2");
        EXPECT_EQ(e2.properties()[1].nameStr(), std::string_view("String"));

        ASSERT_NE(e2.properties()[2].getInt32(), nullptr);
        EXPECT_EQ(*e2.properties()[2].getInt32(), -123);
        EXPECT_EQ(e2.properties()[2].nameStr(), std::string_view("Int32"));

        ASSERT_NE(e2.properties()[3].getUInt64(), nullptr);
        EXPECT_EQ(*e2.properties()[3].getUInt64(), 0x8000000010002000ULL);
        EXPECT_EQ(e2.properties()[3].nameStr(), std::string_view("UInt64"));
    }
}

TEST(Error, log)
{
    // empty
    {
        Error e;
        ErLogInfo("-- empty -----------------------------------");
        logError(Log::get(), Log::Level::Info, e);
        ErLogInfo("--------------------------------------------");
    }

    // not decoded, no brief
    {
        ErLogInfo("-- not decoded, no brief ------------------");

        Error e(
            ENOENT,
            PosixError,
            ErrorProperties::ErrorProperty<std::string, Property::Type::String, "String">("Test string 1"),
            ErrorProperties::ErrorProperty<std::string, Property::Type::String, "String">("Test string 2"),
            ErrorProperties::ErrorProperty<std::int32_t, Property::Type::Int32, "Int32">(-123),
            ErrorProperties::ErrorProperty<std::uint64_t, Property::Type::UInt64, "UInt64">(0x8000000010002000ULL)
        );

        logError(Log::get(), Log::Level::Info, e);
        ErLogInfo("--------------------------------------------");
    }

    // decoded, no brief
    {
        ErLogInfo("------ decoded, no brief -------------------");

        Error e(
            ENOENT,
            PosixError,
            ErrorProperties::ErrorProperty<std::string, Property::Type::String, "String">("Test string 1"),
            ErrorProperties::ErrorProperty<std::string, Property::Type::String, "String">("Test string 2"),
            ErrorProperties::ErrorProperty<std::int32_t, Property::Type::Int32, "Int32">(-123),
            ErrorProperties::ErrorProperty<std::uint64_t, Property::Type::UInt64, "UInt64">(0x8000000010002000ULL)
        );

        e.decode();

        logError(Log::get(), Log::Level::Info, e);
        ErLogInfo("--------------------------------------------");
    }

    // not decoded, brief
    {
        ErLogInfo("------ not decoded, brief ------------------");

        Error e(
            ENOENT,
            PosixError,
            ErrorProperties::Brief("Some description"),
            ErrorProperties::ErrorProperty<std::string, Property::Type::String, "String">("Test string 1"),
            ErrorProperties::ErrorProperty<std::string, Property::Type::String, "String">("Test string 2"),
            ErrorProperties::ErrorProperty<std::int32_t, Property::Type::Int32, "Int32">(-123),
            ErrorProperties::ErrorProperty<std::uint64_t, Property::Type::UInt64, "UInt64">(0x8000000010002000ULL)
        );

        logError(Log::get(), Log::Level::Info, e);
        ErLogInfo("--------------------------------------------");
    }

    // decoded, brief
    {
        ErLogInfo("---------- decoded, brief ------------------");

        Error e(
            ENOENT,
            PosixError,
            ErrorProperties::Brief("Some description"),
            ErrorProperties::ErrorProperty<std::string, Property::Type::String, "String">("Test string 1"),
            ErrorProperties::ErrorProperty<std::string, Property::Type::String, "String">("Test string 2"),
            ErrorProperties::ErrorProperty<std::int32_t, Property::Type::Int32, "Int32">(-123),
            ErrorProperties::ErrorProperty<std::uint64_t, Property::Type::UInt64, "UInt64">(0x8000000010002000ULL)
        );

        e.decode();

        logError(Log::get(), Log::Level::Info, e);
        ErLogInfo("--------------------------------------------");
    }
}

