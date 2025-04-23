#include "common.hpp"

#include <erebus/rtl/reflectable.hxx>

using namespace Er;

ER_REFLECTABLE_IDS_BEGIN(My, MyTraits)
    Zeroth,
    First,
    Second,
    Third
ER_REFLECTABLE_IDS_END()


struct My
    : public Reflectable<MyTraits>
{
    std::int32_t zeroth;
    std::string first;
    std::int64_t second;
    double third;

    ER_REFLECTABLE_FILEDS_BEGIN(My)
        ER_REFLECTABLE_FIELD(My, Zeroth, Semantics::Default, zeroth),
        ER_REFLECTABLE_FIELD(My, First, Semantics::Default, first),
        ER_REFLECTABLE_FIELD(My, Second, Semantics::Default, second),
        ER_REFLECTABLE_FIELD(My, Third, Semantics::Default, third)
    ER_REFLECTABLE_FILEDS_END()
};



TEST(Reflectable, base)
{
    auto& flds = My::fields();
    for (auto& f : flds)
    {
        ErLogDebug("#{}.{}", f.id, f.name);
    }

    My m1;
    EXPECT_FALSE(m1.valid(My::Traits::FieldIds::Zeroth));
    EXPECT_EQ(m1.get<std::int32_t>(My::Traits::FieldIds::Zeroth), nullptr);

    EXPECT_FALSE(m1.valid(My::Traits::FieldIds::First));
    EXPECT_EQ(m1.get<std::string>(My::Traits::FieldIds::First), nullptr);

    EXPECT_FALSE(m1.valid(My::Traits::FieldIds::Second));
    EXPECT_EQ(m1.get<std::int64_t>(My::Traits::FieldIds::Second), nullptr);

    EXPECT_FALSE(m1.valid(My::Traits::FieldIds::Third));
    EXPECT_EQ(m1.get<double>(My::Traits::FieldIds::Third), nullptr);

    m1.set(My::Traits::FieldIds::Zeroth, std::int32_t(-12));
    m1.set(My::Traits::FieldIds::First, std::string("Hello!"));
    m1.set(My::Traits::FieldIds::Second, std::int64_t(12345));
    m1.set(My::Traits::FieldIds::Third, double(-0.9));

    EXPECT_TRUE(m1.valid(My::Traits::FieldIds::Zeroth));
    ASSERT_NE(m1.get<std::int32_t>(My::Traits::FieldIds::Zeroth), nullptr);
    EXPECT_EQ(*m1.get<std::int32_t>(My::Traits::FieldIds::Zeroth), -12);

    EXPECT_TRUE(m1.valid(My::Traits::FieldIds::First));
    ASSERT_NE(m1.get<std::string>(My::Traits::FieldIds::First), nullptr);
    EXPECT_STREQ(m1.get<std::string>(My::Traits::FieldIds::First)->c_str(), "Hello!");

    EXPECT_TRUE(m1.valid(My::Traits::FieldIds::Second));
    ASSERT_NE(m1.get<std::int64_t>(My::Traits::FieldIds::Second), nullptr);
    EXPECT_EQ(*m1.get<std::int64_t>(My::Traits::FieldIds::Second), 12345);

    EXPECT_TRUE(m1.valid(My::Traits::FieldIds::Third));
    ASSERT_NE(m1.get<double>(My::Traits::FieldIds::Third), nullptr);
    EXPECT_DOUBLE_EQ(*m1.get<double>(My::Traits::FieldIds::Third), -0.9);
    

}