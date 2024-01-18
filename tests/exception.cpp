#include "common.hpp"

#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>
#include <erebus/util/exceptionutil.hxx>


TEST(Kes_Exception, simple)
{
    try
    {
        throw Er::Exception(ER_HERE(), "Test exception");
    }
    catch (Er::Exception& e)
    {
        EXPECT_STREQ(e.what(), "Test exception");
    }
}

TEST(Er_Exception, props)
{
    try
    {
        throw Er::Exception(ER_HERE(), "Test exception 2").add(1, "value1").add(2, 1991);
    }
    catch (Er::Exception& e)
    {
        EXPECT_STREQ(e.what(), "Test exception 2");

        auto props = e.properties();
        ASSERT_NE(props, nullptr);
        EXPECT_EQ(props->size(), 2);

        auto p0 = e.find(1);
        ASSERT_NE(p0, nullptr);
        EXPECT_EQ(p0->id, 1);
        EXPECT_STREQ(std::any_cast<const char*>(p0->value), "value1");

        auto p1 = e.find(2);
        ASSERT_NE(p1, nullptr);
        EXPECT_EQ(p1->id, 2);
        EXPECT_EQ(std::any_cast<int>(p1->value), 1991);
    }
}

TEST(Er_Exception, known_props)
{
    try
    {
        throw Er::Exception(ER_HERE(), "POSIX exception", Er::ExceptionProps::PosixErrorCode(ENOENT), Er::ExceptionProps::DecodedError("ENOENT"));
    }
    catch (Er::Exception& e)
    {
        EXPECT_STREQ(e.what(), "POSIX exception");

        auto props = e.properties();
        ASSERT_NE(props, nullptr);
        EXPECT_EQ(props->size(), 2);

        auto code = e.find(Er::ExceptionProps::PosixErrorCode::Id::value);
        ASSERT_NE(code, nullptr);
        EXPECT_EQ(code->id, Er::ExceptionProps::PosixErrorCode::Id::value);
        EXPECT_EQ(std::any_cast<int>(code->value), ENOENT);

        auto text = e.find(Er::ExceptionProps::DecodedError::Id::value);
        ASSERT_NE(text, nullptr);
        EXPECT_EQ(text->id, Er::ExceptionProps::DecodedError::Id::value);
        EXPECT_STREQ(std::any_cast<std::string>(text->value).c_str(), "ENOENT");

    }
}

TEST(Er_Exception, format1)
{
    try
    {
        try
        {
            try
            {
                throw std::bad_alloc();
            }
            catch (std::exception&)
            {
                std::throw_with_nested(std::runtime_error("Level 2 exception"));
            }
        }
        catch (std::exception&)
        {
            std::throw_with_nested(std::runtime_error("Level 3 exception"));
        }
    }
    catch (std::exception& e)
    {
        auto s = Er::Util::formatException(e);
        std::cout << s << "\n";
    }
}

TEST(Er_Exception, format2)
{
    try
    {
        try
        {
            try
            {
                throw std::bad_alloc();
            }
            catch (std::exception& e)
            {
                std::throw_with_nested(
                    Er::Exception(ER_HERE(), "POSIX exception", Er::ExceptionProps::PosixErrorCode(ENOENT), Er::ExceptionProps::DecodedError("No such file"))
                );
            }
        }
        catch (std::exception& e)
        {
            std::throw_with_nested(std::runtime_error("Level 3 exception"));
        }
    }
    catch (std::exception& e)
    {
        auto s = Er::Util::formatException(e);
        std::cout << s << "\n";
    }
}
