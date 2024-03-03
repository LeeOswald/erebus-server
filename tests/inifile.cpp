#include "common.hpp"

#include <erebus/util/inifile.hxx>


static const char SampleIni[] = R"(
; comment 1
#comment 2
# commen 3
[Section 1]
key0=
key1 = value 1
key2=value2
key[3] = value31;value32; comment 4
key4="value4#not a comment#either"

[Section[2]]
key5=#comment5

key6 = value 6 # comment 6
key7=value"not quoted
)";


TEST(IniFile, parse)
{
    auto ini = Er::Util::IniFile::parse(std::string_view(SampleIni));
    ASSERT_EQ(ini.size(), 2);

    auto val = Er::Util::IniFile::lookup(ini, std::string_view("Section 1"), std::string_view("key0"));
    ASSERT_TRUE(val.has_value());
    EXPECT_TRUE(val->empty());

    val = Er::Util::IniFile::lookup(ini, std::string_view("Section"), std::string_view("key0"));
    ASSERT_FALSE(val.has_value());
    
    val = Er::Util::IniFile::lookup(ini, std::string_view("Section 1"), std::string_view("key1"));
    ASSERT_TRUE(val.has_value());
    EXPECT_STREQ(std::string(*val).c_str(), "value 1");

    val = Er::Util::IniFile::lookup(ini, std::string_view("Section 1"), std::string_view("key2"));
    ASSERT_TRUE(val.has_value());
    EXPECT_STREQ(std::string(*val).c_str(), "value2");

    val = Er::Util::IniFile::lookup(ini, std::string_view("Section 1"), std::string_view("key[3]"));
    ASSERT_TRUE(val.has_value());
    EXPECT_STREQ(std::string(*val).c_str(), "value31;value32");

    val = Er::Util::IniFile::lookup(ini, std::string_view("Section 1"), std::string_view("key4"));
    ASSERT_TRUE(val.has_value());
    EXPECT_STREQ(std::string(*val).c_str(), "value4#not a comment#either");

    val = Er::Util::IniFile::lookup(ini, std::string_view("Section[2]"), std::string_view("key5"));
    ASSERT_TRUE(val.has_value());
    EXPECT_TRUE(val->empty());

    val = Er::Util::IniFile::lookup(ini, std::string_view("Section[2]"), std::string_view("key6"));
    ASSERT_TRUE(val.has_value());
    EXPECT_STREQ(std::string(*val).c_str(), "value 6");

    val = Er::Util::IniFile::lookup(ini, std::string_view("Section[2]"), std::string_view("key7"));
    ASSERT_TRUE(val.has_value());
    EXPECT_STREQ(std::string(*val).c_str(), "value\"not quoted");
}