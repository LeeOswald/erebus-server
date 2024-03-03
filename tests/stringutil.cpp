#include "common.hpp"

#include <erebus/util/stringutil.hxx>


TEST(string_split, char_string)
{
    // split empty string
    {
        auto v = Er::Util::split(std::string_view(), std::string_view(" "), Er::Util::SplitSkipEmptyParts);
        EXPECT_TRUE(v.empty());
        v = Er::Util::split(std::string_view(), std::string_view(" "), Er::Util::SplitKeepEmptyParts);
        EXPECT_TRUE(v.empty());
    }

    // split by empty separator
    {
        auto v = Er::Util::split(std::string_view("test string"), std::string_view(), Er::Util::SplitSkipEmptyParts);
        ASSERT_EQ(v.size(), 1);
        EXPECT_STREQ(std::string(v[0]).c_str(), "test string");
        v = Er::Util::split(std::string_view("test string"), std::string_view(), Er::Util::SplitKeepEmptyParts);
        ASSERT_EQ(v.size(), 1);
        EXPECT_STREQ(std::string(v[0]).c_str(), "test string");
    }

    // split by single separator
    {
        auto v = Er::Util::split(std::string_view(";test ;string;;"), std::string_view(";"), Er::Util::SplitSkipEmptyParts);
        ASSERT_EQ(v.size(), 2);
        EXPECT_STREQ(std::string(v[0]).c_str(), "test ");
        EXPECT_STREQ(std::string(v[1]).c_str(), "string");

        v = Er::Util::split(std::string_view(";test ;string;;"), std::string_view(";"), Er::Util::SplitKeepEmptyParts);
        ASSERT_EQ(v.size(), 4);
        EXPECT_STREQ(std::string(v[0]).c_str(), "");
        EXPECT_STREQ(std::string(v[1]).c_str(), "test ");
        EXPECT_STREQ(std::string(v[2]).c_str(), "string");
        EXPECT_STREQ(std::string(v[3]).c_str(), "");
    }

    // split by multiple separator
    {
        auto v = Er::Util::split(std::string_view(";test .string!;"), std::string_view(";.!"), Er::Util::SplitSkipEmptyParts);
        ASSERT_EQ(v.size(), 2);
        EXPECT_STREQ(std::string(v[0]).c_str(), "test ");
        EXPECT_STREQ(std::string(v[1]).c_str(), "string");

        v = Er::Util::split(std::string_view(";test .string!;"), std::string_view(";.!"), Er::Util::SplitKeepEmptyParts);
        ASSERT_EQ(v.size(), 4);
        EXPECT_STREQ(std::string(v[0]).c_str(), "");
        EXPECT_STREQ(std::string(v[1]).c_str(), "test ");
        EXPECT_STREQ(std::string(v[2]).c_str(), "string");
        EXPECT_STREQ(std::string(v[3]).c_str(), "");
    }
}

TEST(string_split, wchar_string)
{
    // split empty string
    {
        auto v = Er::Util::split(std::wstring_view(), std::wstring_view(L" "), Er::Util::SplitSkipEmptyParts);
        EXPECT_TRUE(v.empty());
        v = Er::Util::split(std::wstring_view(), std::wstring_view(L" "), Er::Util::SplitKeepEmptyParts);
        EXPECT_TRUE(v.empty());
    }

    // split by empty separator
    {
        auto v = Er::Util::split(std::wstring_view(L"test string"), std::wstring_view(), Er::Util::SplitSkipEmptyParts);
        ASSERT_EQ(v.size(), 1);
        EXPECT_STREQ(std::wstring(v[0]).c_str(), L"test string");
        v = Er::Util::split(std::wstring_view(L"test string"), std::wstring_view(), Er::Util::SplitKeepEmptyParts);
        ASSERT_EQ(v.size(), 1);
        EXPECT_STREQ(std::wstring(v[0]).c_str(), L"test string");
    }

    // split by single separator
    {
        auto v = Er::Util::split(std::wstring_view(L";test ;string;;"), std::wstring_view(L";"), Er::Util::SplitSkipEmptyParts);
        ASSERT_EQ(v.size(), 2);
        EXPECT_STREQ(std::wstring(v[0]).c_str(), L"test ");
        EXPECT_STREQ(std::wstring(v[1]).c_str(), L"string");

        v = Er::Util::split(std::wstring_view(L";test ;string;;"), std::wstring_view(L";"), Er::Util::SplitKeepEmptyParts);
        ASSERT_EQ(v.size(), 4);
        EXPECT_STREQ(std::wstring(v[0]).c_str(), L"");
        EXPECT_STREQ(std::wstring(v[1]).c_str(), L"test ");
        EXPECT_STREQ(std::wstring(v[2]).c_str(), L"string");
        EXPECT_STREQ(std::wstring(v[3]).c_str(), L"");
    }

    // split by multiple separator
    {
        auto v = Er::Util::split(std::wstring_view(L";test .string!;"), std::wstring_view(L";.!"), Er::Util::SplitSkipEmptyParts);
        ASSERT_EQ(v.size(), 2);
        EXPECT_STREQ(std::wstring(v[0]).c_str(), L"test ");
        EXPECT_STREQ(std::wstring(v[1]).c_str(), L"string");

        v = Er::Util::split(std::wstring_view(L";test .string!;"), std::wstring_view(L";.!"), Er::Util::SplitKeepEmptyParts);
        ASSERT_EQ(v.size(), 4);
        EXPECT_STREQ(std::wstring(v[0]).c_str(), L"");
        EXPECT_STREQ(std::wstring(v[1]).c_str(), L"test ");
        EXPECT_STREQ(std::wstring(v[2]).c_str(), L"string");
        EXPECT_STREQ(std::wstring(v[3]).c_str(), L"");
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