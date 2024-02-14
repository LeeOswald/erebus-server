#include "common.hpp"

#include <erebus/flags.hxx>

struct F : public Er::FlagsBase<70>
{
    static constexpr Flag _0 = 0;
    static constexpr Flag _1 = 1;
    static constexpr Flag _20 = 20;
    static constexpr Flag _69 = 69;

    static const char* name(Flag f) noexcept
    {
        switch (f)
        {
        case _0: return "_0";
        case _1: return "_1";
        case _20: return "_20";
        case _69: return "_69";
        }

        return nullptr;
    }
};

using FF = Er::Flags<F>;

TEST(Flags, simple)
{
    // default constructor
    {
        FF f;
        EXPECT_EQ(f.size(), 70);
        EXPECT_FALSE(f.any());
        EXPECT_TRUE(f.none());
        EXPECT_EQ(f.count(), 0);
        EXPECT_FALSE(f[F::_0]);
        EXPECT_FALSE(f[F::_1]);
        EXPECT_FALSE(f[F::_20]);
        EXPECT_FALSE(f[F::_69]);
        EXPECT_TRUE(f == FF());
        EXPECT_TRUE(f != FF({ FF::_0, FF::_1 }));
    }

    // construct from flags
    {
        FF f{ F::_0, F::_20};
        EXPECT_EQ(f.size(), 70);
        EXPECT_TRUE(f.any());
        EXPECT_FALSE(f.none());
        EXPECT_EQ(f.count(), 2);
        EXPECT_TRUE(f[F::_0]);
        EXPECT_FALSE(f[F::_1]);
        EXPECT_TRUE(f[F::_20]);
        EXPECT_FALSE(f[F::_69]);
        EXPECT_FALSE(f == FF());
        EXPECT_TRUE(f == FF({ FF::_0, FF::_20 }));
    }

    // construct from string
    {
        FF f(std::string_view("100000000000000000010"));
        EXPECT_EQ(f.size(), 70);
        EXPECT_TRUE(f.any());
        EXPECT_FALSE(f.none());
        EXPECT_EQ(f.count(), 2);
        EXPECT_FALSE(f[F::_0]);
        EXPECT_TRUE(f[F::_1]);
        EXPECT_TRUE(f[F::_20]);
        EXPECT_FALSE(f[F::_69]);
        EXPECT_FALSE(f == FF());
        EXPECT_TRUE(f == FF({ FF::_1, FF::_20 }));
    }

    // construct from empty string
    {
        FF f(std::string_view(""));
        EXPECT_EQ(f.size(), 70);
        EXPECT_FALSE(f.any());
        EXPECT_TRUE(f.none());
        EXPECT_EQ(f.count(), 0);
        EXPECT_FALSE(f[F::_0]);
        EXPECT_FALSE(f[F::_1]);
        EXPECT_FALSE(f[F::_20]);
        EXPECT_FALSE(f[F::_69]);
        EXPECT_TRUE(f == FF());
        EXPECT_TRUE(f != FF({ FF::_0, FF::_1 }));
    }

    // convert to string
    {
        std::string source("0000000000000000000000000000000000000000000000000100000000000000000010");
        FF f(source);
        auto dest = f.to_string();
        EXPECT_STREQ(source.c_str(), dest.c_str());
    }
}