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


TEST(Reflectable, FieldInfo)
{
    auto& flds = My::fields();
    for (auto& f : flds)
    {
        auto ty = lookupType(f.type);
        ErLogDebug("[{}] #{}.{}", (ty ? ty->name : std::string_view("?")), f.id, f.name);
    }

    EXPECT_EQ(flds[My::Traits::FieldIds::Zeroth].type, typeId<std::int32_t>().index());
    EXPECT_EQ(flds[My::Traits::FieldIds::Zeroth].id, My::Traits::FieldIds::Zeroth);
    EXPECT_EQ(flds[My::Traits::FieldIds::Zeroth].name, std::string("zeroth"));

    EXPECT_EQ(flds[My::Traits::FieldIds::First].type, typeId<std::string>().index());
    EXPECT_EQ(flds[My::Traits::FieldIds::First].id, My::Traits::FieldIds::First);
    EXPECT_EQ(flds[My::Traits::FieldIds::First].name, std::string("first"));

    EXPECT_EQ(flds[My::Traits::FieldIds::Second].type, typeId<std::int64_t>().index());
    EXPECT_EQ(flds[My::Traits::FieldIds::Second].id, My::Traits::FieldIds::Second);
    EXPECT_EQ(flds[My::Traits::FieldIds::Second].name, std::string("second"));

    EXPECT_EQ(flds[My::Traits::FieldIds::Third].type, typeId<double>().index());
    EXPECT_EQ(flds[My::Traits::FieldIds::Third].id, My::Traits::FieldIds::Third);
    EXPECT_EQ(flds[My::Traits::FieldIds::Third].name, std::string("third"));
}

TEST(Reflectable, base)
{
    My m1;
    EXPECT_FALSE(m1.valid(My::Traits::FieldIds::Zeroth));
    EXPECT_EQ(ErGetp(My, Zeroth, m1, zeroth), nullptr);
    EXPECT_EQ(m1.type(My::Traits::FieldIds::Zeroth), typeId<std::int32_t>().index());

    EXPECT_FALSE(m1.valid(My::Traits::FieldIds::First));
    EXPECT_EQ(ErGetp(My, First, m1, first), nullptr);
    EXPECT_EQ(m1.type(My::Traits::FieldIds::First), typeId<std::string>().index());

    EXPECT_FALSE(m1.valid(My::Traits::FieldIds::Second));
    EXPECT_EQ(ErGetp(My, Second, m1, second), nullptr);
    EXPECT_EQ(m1.type(My::Traits::FieldIds::Second), typeId<std::int64_t>().index());

    EXPECT_FALSE(m1.valid(My::Traits::FieldIds::Third));
    EXPECT_EQ(ErGetp(My, Third, m1, third), nullptr);
    EXPECT_EQ(m1.type(My::Traits::FieldIds::Third), typeId<double>().index());

    ErSet(My, Zeroth, m1, zeroth, -12);
    ErSet(My, First, m1, first, "Hello!");
    ErSet(My, Second, m1, second, 12345);
    ErSet(My, Third, m1, third, -0.9);

    EXPECT_TRUE(m1.valid(My::Traits::FieldIds::Zeroth));
    ASSERT_NE(ErGetp(My, Zeroth, m1, zeroth), nullptr);
    EXPECT_EQ(ErGet(My, Zeroth, m1, zeroth), -12);

    EXPECT_TRUE(m1.valid(My::Traits::FieldIds::First));
    ASSERT_NE(ErGetp(My, First, m1, first), nullptr);
    EXPECT_STREQ(ErGetp(My, First, m1, first)->c_str(), "Hello!");

    EXPECT_TRUE(m1.valid(My::Traits::FieldIds::Second));
    ASSERT_NE(ErGetp(My, Second, m1, second), nullptr);
    EXPECT_EQ(ErGet(My, Second, m1, second), 12345);

    EXPECT_TRUE(m1.valid(My::Traits::FieldIds::Third));
    ASSERT_NE(ErGetp(My, Third, m1, third), nullptr);
    EXPECT_DOUBLE_EQ(ErGet(My, Third, m1, third), -0.9);
    

}