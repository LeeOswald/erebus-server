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

TEST(Reflectable, GetSet)
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

    auto h0 = m1.hash();

    ErSet(My, Zeroth, m1, zeroth, -12);
    EXPECT_NE(m1.hash(), h0);
    h0 = m1.hash();

    ErSet(My, First, m1, first, "Hello!");
    EXPECT_NE(m1.hash(), h0);
    h0 = m1.hash();

    ErSet(My, Second, m1, second, 12345);
    EXPECT_NE(m1.hash(), h0);
    h0 = m1.hash();

    ErSet(My, Third, m1, third, -0.9);
    EXPECT_NE(m1.hash(), h0);

    EXPECT_TRUE(m1.valid(My::Traits::FieldIds::Zeroth));
    ASSERT_NE(ErGetp(My, Zeroth, m1, zeroth), nullptr);
    EXPECT_EQ(ErGet(My, Zeroth, m1, zeroth), m1.zeroth);

    EXPECT_TRUE(m1.valid(My::Traits::FieldIds::First));
    ASSERT_NE(ErGetp(My, First, m1, first), nullptr);
    EXPECT_EQ(ErGet(My, First, m1, first), m1.first);

    EXPECT_TRUE(m1.valid(My::Traits::FieldIds::Second));
    ASSERT_NE(ErGetp(My, Second, m1, second), nullptr);
    EXPECT_EQ(ErGet(My, Second, m1, second), m1.second);

    EXPECT_TRUE(m1.valid(My::Traits::FieldIds::Third));
    ASSERT_NE(ErGetp(My, Third, m1, third), nullptr);
    EXPECT_DOUBLE_EQ(ErGet(My, Third, m1, third), m1.third);
}

TEST(Reflectable, Copy)
{
    My m1;
    ErSet(My, Zeroth, m1, zeroth, 34);
    ErSet(My, First, m1, first, "Bye?");
    ErSet(My, Second, m1, second, -54321);
    ErSet(My, Third, m1, third, 9.99);

    My m2(m1);
    EXPECT_EQ(m2.hash(), m1.hash());

    EXPECT_TRUE(m2.valid(My::Traits::FieldIds::Zeroth));
    ASSERT_NE(ErGetp(My, Zeroth, m2, zeroth), nullptr);
    EXPECT_EQ(ErGet(My, Zeroth, m2, zeroth), m1.zeroth);

    EXPECT_TRUE(m2.valid(My::Traits::FieldIds::First));
    ASSERT_NE(ErGetp(My, First, m2, first), nullptr);
    EXPECT_EQ(ErGet(My, First, m2, first), m1.first);

    EXPECT_TRUE(m2.valid(My::Traits::FieldIds::Second));
    ASSERT_NE(ErGetp(My, Second, m2, second), nullptr);
    EXPECT_EQ(ErGet(My, Second, m2, second), m1.second);

    EXPECT_TRUE(m2.valid(My::Traits::FieldIds::Third));
    ASSERT_NE(ErGetp(My, Third, m2, third), nullptr);
    EXPECT_DOUBLE_EQ(ErGet(My, Third, m2, third), m1.third);

    My m3{};
    EXPECT_EQ(m3.validMask(), My::FieldSet{});

    m3 = m1;
    EXPECT_EQ(m3.hash(), m1.hash());

    EXPECT_TRUE(m3.valid(My::Traits::FieldIds::Zeroth));
    ASSERT_NE(ErGetp(My, Zeroth, m3, zeroth), nullptr);
    EXPECT_EQ(ErGet(My, Zeroth, m3, zeroth), m1.zeroth);

    EXPECT_TRUE(m3.valid(My::Traits::FieldIds::First));
    ASSERT_NE(ErGetp(My, First, m3, first), nullptr);
    EXPECT_EQ(ErGet(My, First, m3, first), m1.first);

    EXPECT_TRUE(m3.valid(My::Traits::FieldIds::Second));
    ASSERT_NE(ErGetp(My, Second, m3, second), nullptr);
    EXPECT_EQ(ErGet(My, Second, m3, second), m1.second);

    EXPECT_TRUE(m3.valid(My::Traits::FieldIds::Third));
    ASSERT_NE(ErGetp(My, Third, m3, third), nullptr);
    EXPECT_DOUBLE_EQ(ErGet(My, Third, m3, third), m1.third);
}

TEST(Reflectable, Move)
{
    My m1;
    ErSet(My, Zeroth, m1, zeroth, 34);
    ErSet(My, First, m1, first, "Bye?");
    ErSet(My, Second, m1, second, -54321);
    ErSet(My, Third, m1, third, 9.99);
    
    My m2(m1);

    My m3(std::move(m1));
    EXPECT_EQ(m1.validMask(), My::FieldSet{});

    EXPECT_EQ(m2.hash(), m2.hash());
    EXPECT_EQ(m3.validMask(), m2.validMask());
    EXPECT_EQ(m3.zeroth, m2.zeroth);
    EXPECT_EQ(m3.first, m2.first);
    EXPECT_EQ(m3.second, m2.second);
    EXPECT_EQ(m3.third, m2.third);

    My m4{};
    m4 = std::move(m3);
    EXPECT_EQ(m3.validMask(), My::FieldSet{});

    EXPECT_EQ(m4.hash(), m2.hash());
    EXPECT_EQ(m4.validMask(), m2.validMask());
    EXPECT_EQ(m4.zeroth, m2.zeroth);
    EXPECT_EQ(m4.first, m2.first);
    EXPECT_EQ(m4.second, m2.second);
    EXPECT_EQ(m4.third, m2.third);
}

TEST(Reflectable, Diff)
{
    My m1;
    ErSet(My, Zeroth, m1, zeroth, 34);
    ErSet(My, First, m1, first, "Bye?");
    ErSet(My, Second, m1, second, -54321);
    ErSet(My, Third, m1, third, 9.99);

    // diff with self
    auto d = m1.diff(m1);
    EXPECT_EQ(d.differences, 0);
    EXPECT_EQ(d.map[My::Traits::FieldIds::Zeroth], My::Diff::Type::Unchanged);
    EXPECT_EQ(d.map[My::Traits::FieldIds::First], My::Diff::Type::Unchanged);
    EXPECT_EQ(d.map[My::Traits::FieldIds::Second], My::Diff::Type::Unchanged);
    EXPECT_EQ(d.map[My::Traits::FieldIds::Third], My::Diff::Type::Unchanged);

    // diff with empty
    My m2;
    d = m2.diff(m1);
    EXPECT_EQ(d.differences, 4);
    EXPECT_EQ(d.map[My::Traits::FieldIds::Zeroth], My::Diff::Type::Added);
    EXPECT_EQ(d.map[My::Traits::FieldIds::First], My::Diff::Type::Added);
    EXPECT_EQ(d.map[My::Traits::FieldIds::Second], My::Diff::Type::Added);
    EXPECT_EQ(d.map[My::Traits::FieldIds::Third], My::Diff::Type::Added);

    d = m1.diff(m2);
    EXPECT_EQ(d.differences, 4);
    EXPECT_EQ(d.map[My::Traits::FieldIds::Zeroth], My::Diff::Type::Removed);
    EXPECT_EQ(d.map[My::Traits::FieldIds::First], My::Diff::Type::Removed);
    EXPECT_EQ(d.map[My::Traits::FieldIds::Second], My::Diff::Type::Removed);
    EXPECT_EQ(d.map[My::Traits::FieldIds::Third], My::Diff::Type::Removed);

    My m3(m1);
    ErSet(My, First, m1, first, "Hello!");
    ErSet(My, Second, m1, second, 12345);
    EXPECT_NE(m3.hash(), m1.hash());
    d = m1.diff(m3);
    EXPECT_EQ(d.differences, 2);
    EXPECT_EQ(d.map[My::Traits::FieldIds::Zeroth], My::Diff::Type::Unchanged);
    EXPECT_EQ(d.map[My::Traits::FieldIds::First], My::Diff::Type::Changed);
    EXPECT_EQ(d.map[My::Traits::FieldIds::Second], My::Diff::Type::Changed);
    EXPECT_EQ(d.map[My::Traits::FieldIds::Third], My::Diff::Type::Unchanged);
}