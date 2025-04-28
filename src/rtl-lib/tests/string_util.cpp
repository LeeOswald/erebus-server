#include "common.hpp"

#include <erebus/rtl/util/string_util.hxx>

using namespace Er::Util;


TEST(string_trim, char_string)
{
    // trim empty string
    {
        std::string s;
        auto t = trim(s);
        EXPECT_TRUE(t.empty());
    }

    // trim untrimmable
    {
        std::string s("untrimmable");
        auto t = trim(s);
        EXPECT_STREQ(t.c_str(), s.c_str());
    }

    // trim
    {
        std::string s(" \r\n\tnormal\n string\t \n ");
        auto t = trim(s);
        EXPECT_STREQ(t.c_str(), "normal\n string");
    }
}

TEST(string_trim, wchar_string)
{
    // trim empty string
    {
        std::wstring s;
        auto t = trim(s);
        EXPECT_TRUE(t.empty());
    }

    // trim untrimmable
    {
        std::wstring s(L"untrimmable");
        auto t = trim(s);
        EXPECT_STREQ(t.c_str(), s.c_str());
    }

    // trim
    {
        std::wstring s(L" \r\n\tnormal\n string\t \n ");
        auto t = trim(s);
        EXPECT_STREQ(t.c_str(), L"normal\n string");
    }
}