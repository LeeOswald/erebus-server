#include "common.hpp"

#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/util/exception_util.hxx>


using namespace Er;


TEST(Exception, construction)
{
    // code only
    {
        Exception e(std::source_location::current(), Error{ ENOENT, PosixError });
        EXPECT_TRUE(e);
        EXPECT_EQ(e.code(), ENOENT);
        EXPECT_EQ(e.category(), PosixError);
        EXPECT_TRUE(e.properties().empty());
    }

    // add props in the constructor
    {
        ExceptionProperty<std::string, Property::Type::String, "String"> prop1("Test string 1");

        EXPECT_EQ(prop1.type(), Property::Type::String);
        EXPECT_EQ(prop1.nameStr(), std::string_view("String"));
        EXPECT_STREQ(prop1.getString()->c_str(), "Test string 1");

        ExceptionProperty<std::string, Property::Type::String, "String"> prop2("Test string 2");

        EXPECT_EQ(prop2.type(), Property::Type::String);
        EXPECT_EQ(prop2.nameStr(), std::string_view("String"));
        EXPECT_STREQ(prop2.getString()->c_str(), "Test string 2");

        ExceptionProperty<std::int32_t, Property::Type::Int32, "Int32"> prop3(-123);

        EXPECT_EQ(prop3.type(), Property::Type::Int32);
        EXPECT_EQ(prop3.nameStr(), std::string_view("Int32"));
        EXPECT_EQ(*prop3.getInt32(), -123);

        ExceptionProperty<std::uint64_t, Property::Type::UInt64, "UInt64"> prop4(std::uint64_t{0x8000000010002000ULL}, Semantics::Pointer);

        EXPECT_EQ(prop4.type(), Property::Type::UInt64);
        EXPECT_EQ(prop4.nameStr(), std::string_view("UInt64"));
        EXPECT_EQ(*prop4.getUInt64(), 0x8000000010002000ULL);

        Exception e(std::source_location::current(), Error{ ENOENT, PosixError }, prop1, std::move(prop2), prop3, std::move(prop4));

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
        ExceptionProperty<std::string, Property::Type::String, "String"> prop1("Test string 1");
        ExceptionProperty<std::string, Property::Type::String, "String"> prop2("Test string 2");
        ExceptionProperty<std::int32_t, Property::Type::Int32, "Int32"> prop3(-123);
        ExceptionProperty<std::uint64_t, Property::Type::UInt64, "UInt64"> prop4(std::uint64_t{0x8000000010002000ULL}, Semantics::Pointer);

        Exception e(std::source_location::current(), Error{ ENOENT, PosixError });
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

TEST(Exception, copymove)
{
    // copy-construct
    {
        Exception e1(
            std::source_location::current(),
            Error{ ENOENT, PosixError },
            ExceptionProperty<std::string, Property::Type::String, "String">("Test string 1"),
            ExceptionProperty<std::string, Property::Type::String, "String">("Test string 2"),
            ExceptionProperty<std::int32_t, Property::Type::Int32, "Int32">(-123),
            ExceptionProperty<std::uint64_t, Property::Type::UInt64, "UInt64">(std::uint64_t{0x8000000010002000ULL}, Semantics::Pointer)
        );

        Exception e2(e1);

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
        Exception e1(
            std::source_location::current(),
            Error{ ENOENT, PosixError },
            ExceptionProperty<std::string, Property::Type::String, "String">("Test string 1"),
            ExceptionProperty<std::string, Property::Type::String, "String">("Test string 2"),
            ExceptionProperty<std::int32_t, Property::Type::Int32, "Int32">(-123),
            ExceptionProperty<std::uint64_t, Property::Type::UInt64, "UInt64">(std::uint64_t{0x8000000010002000ULL}, Semantics::Pointer)
        );

        Exception e2(
            std::source_location::current(),
            Error{ EINVAL, PosixError },
            ExceptionProperty<double, Property::Type::Double, "Double">(-0.9)
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
        Exception e1(
            std::source_location::current(),
            Error{ ENOENT, PosixError },
            ExceptionProperty<std::string, Property::Type::String, "String">("Test string 1"),
            ExceptionProperty<std::string, Property::Type::String, "String">("Test string 2"),
            ExceptionProperty<std::int32_t, Property::Type::Int32, "Int32">(-123),
            ExceptionProperty<std::uint64_t, Property::Type::UInt64, "UInt64">(std::uint64_t{0x8000000010002000ULL}, Semantics::Pointer)
        );

        Exception e2(std::move(e1));

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
        Exception e1(
            std::source_location::current(),
            Error{ ENOENT, PosixError },
            ExceptionProperty<std::string, Property::Type::String, "String">("Test string 1"),
            ExceptionProperty<std::string, Property::Type::String, "String">("Test string 2"),
            ExceptionProperty<std::int32_t, Property::Type::Int32, "Int32">(-123),
            ExceptionProperty<std::uint64_t, Property::Type::UInt64, "UInt64">(std::uint64_t{0x8000000010002000ULL}, Semantics::Pointer)
        );

        Exception e2(
            std::source_location::current(),
            Error{ EINVAL, PosixError },
            ExceptionProperty<double, Property::Type::Double, "Double">(-0.9)
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

TEST(Exception, log)
{
    // no brief
    {
        Exception e(
            std::source_location::current(),
            Error{ ENOENT, PosixError },
            ExceptionProperty<std::string, Property::Type::String, "String">("Test string 1"),
            ExceptionProperty<std::string, Property::Type::String, "String">("Test string 2"),
            ExceptionProperty<std::int32_t, Property::Type::Int32, "Int32">(-123),
            ExceptionProperty<std::uint64_t, Property::Type::UInt64, "UInt64">(std::uint64_t{0x8000000010002000ULL}, Semantics::Pointer)
        );

        Util::logException(Log::get(), Log::Level::Info, e);
    }

    // brief
    {
        Exception e(
            std::source_location::current(),
            Error{ ENOENT, PosixError },
            ExceptionProperties::Brief("Some description"),
            ExceptionProperty<std::string, Property::Type::String, "String">("Test string 1"),
            ExceptionProperty<std::string, Property::Type::String, "String">("Test string 2"),
            ExceptionProperty<std::int32_t, Property::Type::Int32, "Int32">(-123),
            ExceptionProperty<std::uint64_t, Property::Type::UInt64, "UInt64">(std::uint64_t{0x8000000010002000ULL}, Semantics::Pointer)
        );

        Util::logException(Log::get(), Log::Level::Info, e);
    }
}

TEST(Exception, throw)
{
    try
    {
        throw Exception(
            std::source_location::current(),
            Error{ ENOENT, PosixError },
            ExceptionProperties::Brief("Test exception"),
            ExceptionProperty<std::string, Property::Type::String, "String">("Test string 1"),
            ExceptionProperty<std::string, Property::Type::String, "String">("Test string 2"),
            ExceptionProperty<std::int32_t, Property::Type::Int32, "Int32">(-123),
            ExceptionProperty<std::uint64_t, Property::Type::UInt64, "UInt64">(std::uint64_t{ 0x8000000010002000ULL }, Semantics::Pointer)
        );
    }
    catch (const Exception& e)
    {
        Util::logException(Log::get(), Log::Level::Info, e);
    }
}