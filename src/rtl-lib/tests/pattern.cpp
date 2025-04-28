#include "common.hpp"

#include <erebus/rtl/util/pattern.hxx>

using namespace Er::Util;


TEST(matchString, All)
{
    EXPECT_TRUE(matchString(std::string_view{}, std::string_view{}));

    EXPECT_FALSE(matchString(std::string_view{}, std::string_view{ "?" }));
    EXPECT_TRUE(matchString(std::string_view{}, std::string_view{ "*" }));

    EXPECT_FALSE(matchString(std::string_view{ "Some text" }, std::string_view{ "" }));

    EXPECT_FALSE(matchString(std::string_view{ "Some text" }, std::string_view{ "?" }));
    EXPECT_TRUE(matchString(std::string_view{ "Some text" }, std::string_view{ "*" }));

    EXPECT_TRUE(matchString(std::string_view{ "Some text" }, std::string_view{ "?ome text" }));
    EXPECT_FALSE(matchString(std::string_view{ "ome text" }, std::string_view{ "?ome text" }));
    EXPECT_TRUE(matchString(std::string_view{ "Some text" }, std::string_view{ "??me text" }));
    EXPECT_TRUE(matchString(std::string_view{ "Some text" }, std::string_view{ "?????????" }));
    EXPECT_FALSE(matchString(std::string_view{ "Some text" }, std::string_view{ "????????" }));

    EXPECT_TRUE(matchString(std::string_view{ "Some text" }, std::string_view{ "* text" }));
    EXPECT_TRUE(matchString(std::string_view{ "Some text" }, std::string_view{ "Some* text" }));
    EXPECT_TRUE(matchString(std::string_view{ "Some text" }, std::string_view{ "Some *" }));
    EXPECT_TRUE(matchString(std::string_view{ "Some text" }, std::string_view{ "**" }));
    EXPECT_TRUE(matchString(std::string_view{ "Some text" }, std::string_view{ "***" }));
    EXPECT_FALSE(matchString(std::string_view{ "Some text" }, std::string_view{ "W*me text" }));
}
