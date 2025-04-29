#include "common.hpp"

#include <erebus/rtl/util/string_util.hxx>

using namespace Er::Util;


TEST(StringUtil, trim)
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

TEST(StringUtil, wtrim)
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

TEST(StringUtil, split)
{
    {
        // empty
        std::string m;

        std::string out;
        split(m, ';', [&out](const char* v, std::size_t len) { out.append(std::string_view(v, len)); });

        EXPECT_TRUE(out.empty());
    }

    {
        // contains only a separator
        std::string m(";");

        std::string out;
        split(m, ';', [&out](const char* v, std::size_t len) { out.append(std::string_view(v, len)); });

        EXPECT_TRUE(out.empty());
    }

    {
        // contains only separators
        std::string m(";;;");

        std::string out;
        split(m, ';', [&out](const char* v, std::size_t len) { out.append(std::string_view(v, len)); });

        EXPECT_TRUE(out.empty());
    }

    {
        // no separator
        std::string m("sample string");

        std::string out;
        split(m, ';', [&out](const char* v, std::size_t len) { out.append(std::string_view(v, len)); });

        EXPECT_EQ(out.length(), m.length());
        EXPECT_EQ(out, std::string("sample string"));
    }

    {
        std::string m(";second;;fourth;");

        std::string out;
        unsigned count = 0;
        split(m, ';', [&out, &count](const char* v, std::size_t len) { ++count; out.append(std::string_view(v, len)); });

        EXPECT_EQ(count, 5);
        EXPECT_EQ(out, std::string("secondfourth"));
    }

    {
        std::string m("a;b");

        std::string out;
        unsigned count = 0;
        split(m, ';', [&out, &count](const char* v, std::size_t len) { ++count; out.append(std::string_view(v, len)); });

        EXPECT_EQ(count, 2);
        EXPECT_EQ(out, std::string("ab"));
    }
}
