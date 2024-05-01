#if !ER_ENABLE_ASSERT
    #define ER_ENABLE_ASSERT 1
#endif

#include "common.hpp"

#include <erebus/exception.hxx>
#include <erebus/util/exceptionutil.hxx>


TEST(Er_Assert, simple)
{
    EXPECT_NO_THROW(ErAssert(true));
    EXPECT_THROW(ErAssert(false), Er::Exception);

    try
    {
        ErAssert(!"Test assertion");
    }
    catch (Er::Exception& e)
    {
        auto prop = e.find(Er::ExceptionProps::FailedAssertion::Id::value);
        ASSERT_TRUE(prop);

        auto a = std::get_if<std::string>(&prop->value);
        ASSERT_TRUE(a);
        EXPECT_STREQ(a->c_str(), "!\"Test assertion\"");

        Er::Util::logException(g_log, Er::Log::Level::Info, e);
    }
}