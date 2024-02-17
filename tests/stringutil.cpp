#include "common.hpp"

#include <erebus/util/stringutil.hxx>


TEST(string_split_by_char, char_string)
{
    // split empty string
    {
        std::string_view s;
        auto v = Er::Util::split(s, ':');
        EXPECT_TRUE(v.empty());
    }

    // unsplittable
    {
        std::string s("unsplittable string");
        auto v = Er::Util::split(s, ':');
        ASSERT_EQ(v.size(), 1);
        EXPECT_STREQ(v.front().c_str(), s.c_str());
    }

    // empty parts
    {
        std::string s(":splittable string:");
        auto v = Er::Util::split(s, ':');
        ASSERT_EQ(v.size(), 3);
        EXPECT_STREQ(v[0].c_str(), "");
        EXPECT_STREQ(v[1].c_str(), "splittable string");
        EXPECT_STREQ(v[2].c_str(), "");
    }

    // normal
    {
        std::string s("splittable string");
        auto v = Er::Util::split(s, ' ');
        ASSERT_EQ(v.size(), 2);
        EXPECT_STREQ(v[0].c_str(), "splittable");
        EXPECT_STREQ(v[1].c_str(), "string");
    }
}

TEST(string_split_by_string, char_string)
{
    // split empty string
    {
        std::string_view s;
        auto v = Er::Util::split(s, "::", 2);
        EXPECT_TRUE(v.empty());
    }

    // unsplittable
    {
        std::string s("unsplittable string");
        auto v = Er::Util::split(s, "::", 2);
        ASSERT_EQ(v.size(), 1);
        EXPECT_STREQ(v.front().c_str(), s.c_str());
    }

    // empty parts
    {
        std::string s("::splittable string::");
        auto v = Er::Util::split(s, "::", 2);
        ASSERT_EQ(v.size(), 3);
        EXPECT_STREQ(v[0].c_str(), "");
        EXPECT_STREQ(v[1].c_str(), "splittable string");
        EXPECT_STREQ(v[2].c_str(), "");
    }

    // normal
    {
        std::string s("splittable::string");
        auto v = Er::Util::split(s, "::", 2);
        ASSERT_EQ(v.size(), 2);
        EXPECT_STREQ(v[0].c_str(), "splittable");
        EXPECT_STREQ(v[1].c_str(), "string");
    }
}

TEST(string_split_by_char, wchar_string)
{
    // split empty string
    {
        std::wstring_view s;
        auto v = Er::Util::split(s, L':');
        EXPECT_TRUE(v.empty());
    }

    // unsplittable
    {
        std::wstring s(L"unsplittable string");
        auto v = Er::Util::split(s, L':');
        ASSERT_EQ(v.size(), 1);
        EXPECT_STREQ(v.front().c_str(), s.c_str());
    }

    // empty parts
    {
        std::wstring s(L":splittable string:");
        auto v = Er::Util::split(s, L':');
        ASSERT_EQ(v.size(), 3);
        EXPECT_STREQ(v[0].c_str(), L"");
        EXPECT_STREQ(v[1].c_str(), L"splittable string");
        EXPECT_STREQ(v[2].c_str(), L"");
    }

    // normal
    {
        std::wstring s(L"splittable string");
        auto v = Er::Util::split(s, L' ');
        ASSERT_EQ(v.size(), 2);
        EXPECT_STREQ(v[0].c_str(), L"splittable");
        EXPECT_STREQ(v[1].c_str(), L"string");
    }
}

TEST(string_split_by_string, wchar_string)
{
    // split empty string
    {
        std::wstring_view s;
        auto v = Er::Util::split(s, L"::", 2);
        EXPECT_TRUE(v.empty());
    }

    // unsplittable
    {
        std::wstring s(L"unsplittable string");
        auto v = Er::Util::split(s, L"::", 2);
        ASSERT_EQ(v.size(), 1);
        EXPECT_STREQ(v.front().c_str(), s.c_str());
    }

    // empty parts
    {
        std::wstring s(L"::splittable string::");
        auto v = Er::Util::split(s, L"::", 2);
        ASSERT_EQ(v.size(), 3);
        EXPECT_STREQ(v[0].c_str(), L"");
        EXPECT_STREQ(v[1].c_str(), L"splittable string");
        EXPECT_STREQ(v[2].c_str(), L"");
    }

    // normal
    {
        std::wstring s(L"splittable::string");
        auto v = Er::Util::split(s, L"::", 2);
        ASSERT_EQ(v.size(), 2);
        EXPECT_STREQ(v[0].c_str(), L"splittable");
        EXPECT_STREQ(v[1].c_str(), L"string");
    }
}

TEST(string_trim, char_string)
{
    // trim empty string
    {
        std::string s;
        auto t = Er::Util::trim(s);
        EXPECT_TRUE(t.empty());
    }

    // trim untrimmable
    {
        std::string s("untrimmable");
        auto t = Er::Util::trim(s);
        EXPECT_STREQ(t.c_str(), s.c_str());
    }

    // trim
    {
        std::string s(" \r\n\tnormal\n string\t \n ");
        auto t = Er::Util::trim(s);
        EXPECT_STREQ(t.c_str(), "normal\n string");
    }
}

TEST(string_trim, wchar_string)
{
    // trim empty string
    {
        std::wstring s;
        auto t = Er::Util::trim(s);
        EXPECT_TRUE(t.empty());
    }

    // trim untrimmable
    {
        std::wstring s(L"untrimmable");
        auto t = Er::Util::trim(s);
        EXPECT_STREQ(t.c_str(), s.c_str());
    }

    // trim
    {
        std::wstring s(L" \r\n\tnormal\n string\t \n ");
        auto t = Er::Util::trim(s);
        EXPECT_STREQ(t.c_str(), L"normal\n string");
    }
}

TEST(CharTraitsIgnoreCase, char_string)
{
    std::basic_string_view<char, Er::Util::CharTraitsIgnoreCase<char>> s1("sample string");
    std::basic_string_view<char, Er::Util::CharTraitsIgnoreCase<char>> s2("SAMPLE STRING");
    std::basic_string_view<char, Er::Util::CharTraitsIgnoreCase<char>> s3("wrong string");

    EXPECT_TRUE(s1 == s2);
    EXPECT_FALSE(s1 == s3);
    EXPECT_NE(s1.find("STRING"), s1.npos); 
    EXPECT_NE(s2.find("string"), s2.npos);
    EXPECT_EQ(s3.find("sample"), s3.npos);
}

TEST(CharTraitsIgnoreCase, wchar_string)
{
    std::basic_string_view<wchar_t, Er::Util::CharTraitsIgnoreCase<wchar_t>> s1(L"sample string");
    std::basic_string_view<wchar_t, Er::Util::CharTraitsIgnoreCase<wchar_t>> s2(L"SAMPLE STRING");
    std::basic_string_view<wchar_t, Er::Util::CharTraitsIgnoreCase<wchar_t>> s3(L"wrong string");

    EXPECT_TRUE(s1 == s2);
    EXPECT_FALSE(s1 == s3);
    EXPECT_NE(s1.find(L"STRING"), s1.npos);
    EXPECT_NE(s2.find(L"string"), s2.npos);
    EXPECT_EQ(s3.find(L"sample"), s3.npos);
}