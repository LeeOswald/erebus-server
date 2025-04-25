#include "common.hpp"

#include <erebus/rtl/multi_string.hxx>

using namespace Er;


TEST(MultiString, coalesce)
{
    {
        // empty
        MultiStringZ m;
        auto s = m.coalesce('.');
        EXPECT_TRUE(s.empty());
    }

    {
        // contains only a separator
        MultiString<';'> m(";");
        auto s = m.coalesce('.');
        EXPECT_EQ(s.length(), m.raw.length());
        EXPECT_EQ(s, std::string("."));
    }

    {
        // contains only separators
        MultiString<';'> m(";;;");
        auto s = m.coalesce('.');
        EXPECT_EQ(s.length(), m.raw.length());
        EXPECT_EQ(s, std::string("..."));
    }

    {
        // no separator
        MultiString<';'> m("sample string");
        auto s = m.coalesce('.');
        EXPECT_EQ(s.length(), m.raw.length());
        EXPECT_EQ(s, std::string("sample string"));
    }

    {
        MultiString<';'> m(";second;;fourth;");
        auto s = m.coalesce('.');
        EXPECT_EQ(s.length(), m.raw.length());
        EXPECT_EQ(s, std::string(".second..fourth."));
    }

    {
        MultiString<';'> m("a;b");
        auto s = m.coalesce('.');
        EXPECT_EQ(s.length(), m.raw.length());
        EXPECT_EQ(s, std::string("a.b"));
    }
}

TEST(MultiString, split)
{
    {
        // empty
        MultiStringZ m;
              
        std::string out;
        m.split([&out](const char* v, std::size_t len) { out.append(std::string_view(v, len)); });

        EXPECT_TRUE(out.empty());
    }

    {
        // contains only a separator
        MultiString<';'> m(";");
        
        std::string out;
        m.split([&out](const char* v, std::size_t len) { out.append(std::string_view(v, len)); });

        EXPECT_TRUE(out.empty());
    }

    {
        // contains only separators
        MultiString<';'> m(";;;");
        
        std::string out;
        m.split([&out](const char* v, std::size_t len) { out.append(std::string_view(v, len)); });

        EXPECT_TRUE(out.empty());
    }

    {
        // no separator
        MultiString<';'> m("sample string");

        std::string out;
        m.split([&out](const char* v, std::size_t len) { out.append(std::string_view(v, len)); });

        EXPECT_EQ(out.length(), m.raw.length());
        EXPECT_EQ(out, std::string("sample string"));
    }

    {
        MultiString<';'> m(";second;;fourth;");
        
        std::string out;
        unsigned count = 0;
        m.split([&out, &count](const char* v, std::size_t len) { ++count; out.append(std::string_view(v, len)); });

        EXPECT_EQ(count, 5);
        EXPECT_EQ(out, std::string("secondfourth"));
    }

    {
        MultiString<';'> m("a;b");

        std::string out;
        unsigned count = 0;
        m.split([&out, &count](const char* v, std::size_t len) { ++count; out.append(std::string_view(v, len)); });

        EXPECT_EQ(count, 2);
        EXPECT_EQ(out, std::string("ab"));
    }
}
