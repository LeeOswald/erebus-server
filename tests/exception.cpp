#include "common.hpp"

#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>
#include <erebus/util/exceptionutil.hxx>


TEST(Er_Exception, simple)
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
        EXPECT_EQ(Er::get<Er::ExceptionProps::PosixErrorCode::ValueType>(code->value), ENOENT);

        auto text = e.find(Er::ExceptionProps::DecodedError::Id::value);
        ASSERT_NE(text, nullptr);
        EXPECT_EQ(text->id, Er::ExceptionProps::DecodedError::Id::value);
        EXPECT_EQ(Er::get<Er::ExceptionProps::DecodedError::ValueType>(text->value), std::string_view("ENOENT"));

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
        Er::Util::logException(Er::Log::defaultLog(), Er::Log::Level::Info, e);
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
        Er::Util::logException(Er::Log::defaultLog(), Er::Log::Level::Info, e);
    }
}
