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

