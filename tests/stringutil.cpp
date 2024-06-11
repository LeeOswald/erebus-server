#include "common.hpp"

#include <erebus/util/stringutil.hxx>



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