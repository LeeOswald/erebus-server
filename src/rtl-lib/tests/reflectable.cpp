#include "common.hpp"

#include <erebus/rtl/reflectable.hxx>

#include <bit>

using namespace Er;


struct My
    : public Reflectable<My, 4>
{
    enum Field : FieldId
    {
        Zeroth,
        First,
        Second,
        Third
    };

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
        ErLogInfo("[{}] #{}.{}", (ty ? ty->name : std::string_view("?")), f.id, f.name);
    }

    EXPECT_EQ(flds[My::Zeroth].type, typeId<std::int32_t>().index());
    EXPECT_EQ(flds[My::Zeroth].id, My::Zeroth);
    EXPECT_EQ(flds[My::Zeroth].name, std::string("zeroth"));

    EXPECT_EQ(flds[My::First].type, typeId<std::string>().index());
    EXPECT_EQ(flds[My::First].id, My::First);
    EXPECT_EQ(flds[My::First].name, std::string("first"));

    EXPECT_EQ(flds[My::Second].type, typeId<std::int64_t>().index());
    EXPECT_EQ(flds[My::Second].id, My::Second);
    EXPECT_EQ(flds[My::Second].name, std::string("second"));

    EXPECT_EQ(flds[My::Third].type, typeId<double>().index());
    EXPECT_EQ(flds[My::Third].id, My::Third);
    EXPECT_EQ(flds[My::Third].name, std::string("third"));
}

TEST(Reflectable, GetSet)
{
    My m1;
    EXPECT_FALSE(m1.valid(My::Zeroth));
    EXPECT_EQ(ErGetp(My, Zeroth, m1, zeroth), nullptr);
    EXPECT_EQ(m1.type(My::Zeroth), typeId<std::int32_t>().index());

    EXPECT_FALSE(m1.valid(My::First));
    EXPECT_EQ(ErGetp(My, First, m1, first), nullptr);
    EXPECT_EQ(m1.type(My::First), typeId<std::string>().index());

    EXPECT_FALSE(m1.valid(My::Second));
    EXPECT_EQ(ErGetp(My, Second, m1, second), nullptr);
    EXPECT_EQ(m1.type(My::Second), typeId<std::int64_t>().index());

    EXPECT_FALSE(m1.valid(My::Third));
    EXPECT_EQ(ErGetp(My, Third, m1, third), nullptr);
    EXPECT_EQ(m1.type(My::Third), typeId<double>().index());

    auto h0 = m1.hash();

    ErSet(My, Zeroth, m1, zeroth, -12);
    EXPECT_NE(m1.hash(), h0);
    h0 = m1.hash();

    std::string s("Hello!");
    ErSet(My, First, m1, first, std::move(s));
    EXPECT_STREQ(m1.first.c_str(), "Hello!");
    EXPECT_TRUE(s.empty()); // moved from

    std::string s1("Bye?");
    ErSet(My, First, m1, first, s1);
    EXPECT_STREQ(m1.first.c_str(), "Bye?");
    EXPECT_FALSE(s1.empty()); // copied from

    EXPECT_NE(m1.hash(), h0);
    h0 = m1.hash();

    ErSet(My, Second, m1, second, 12345);
    EXPECT_NE(m1.hash(), h0);
    h0 = m1.hash();

    ErSet(My, Third, m1, third, -0.9);
    EXPECT_NE(m1.hash(), h0);

    EXPECT_TRUE(m1.valid(My::Zeroth));
    ASSERT_NE(ErGetp(My, Zeroth, m1, zeroth), nullptr);
    EXPECT_EQ(ErGet(My, Zeroth, m1, zeroth), m1.zeroth);

    EXPECT_TRUE(m1.valid(My::First));
    ASSERT_NE(ErGetp(My, First, m1, first), nullptr);
    EXPECT_EQ(ErGet(My, First, m1, first), m1.first);

    EXPECT_TRUE(m1.valid(My::Second));
    ASSERT_NE(ErGetp(My, Second, m1, second), nullptr);
    EXPECT_EQ(ErGet(My, Second, m1, second), m1.second);

    EXPECT_TRUE(m1.valid(My::Third));
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

    EXPECT_TRUE(m2.valid(My::Zeroth));
    ASSERT_NE(ErGetp(My, Zeroth, m2, zeroth), nullptr);
    EXPECT_EQ(ErGet(My, Zeroth, m2, zeroth), m1.zeroth);

    EXPECT_TRUE(m2.valid(My::First));
    ASSERT_NE(ErGetp(My, First, m2, first), nullptr);
    EXPECT_EQ(ErGet(My, First, m2, first), m1.first);

    EXPECT_TRUE(m2.valid(My::Second));
    ASSERT_NE(ErGetp(My, Second, m2, second), nullptr);
    EXPECT_EQ(ErGet(My, Second, m2, second), m1.second);

    EXPECT_TRUE(m2.valid(My::Third));
    ASSERT_NE(ErGetp(My, Third, m2, third), nullptr);
    EXPECT_DOUBLE_EQ(ErGet(My, Third, m2, third), m1.third);

    My m3{};
    EXPECT_EQ(m3.validMask(), My::FieldSet{});

    m3 = m1;
    EXPECT_EQ(m3.hash(), m1.hash());

    EXPECT_TRUE(m3.valid(My::Zeroth));
    ASSERT_NE(ErGetp(My, Zeroth, m3, zeroth), nullptr);
    EXPECT_EQ(ErGet(My, Zeroth, m3, zeroth), m1.zeroth);

    EXPECT_TRUE(m3.valid(My::First));
    ASSERT_NE(ErGetp(My, First, m3, first), nullptr);
    EXPECT_EQ(ErGet(My, First, m3, first), m1.first);

    EXPECT_TRUE(m3.valid(My::Second));
    ASSERT_NE(ErGetp(My, Second, m3, second), nullptr);
    EXPECT_EQ(ErGet(My, Second, m3, second), m1.second);

    EXPECT_TRUE(m3.valid(My::Third));
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
    EXPECT_TRUE(m1.first.empty()); // moved away

    EXPECT_EQ(m2.hash(), m2.hash());
    EXPECT_EQ(m3.validMask(), m2.validMask());
    EXPECT_EQ(m3.zeroth, m2.zeroth);
    EXPECT_EQ(m3.first, m2.first);
    EXPECT_EQ(m3.second, m2.second);
    EXPECT_EQ(m3.third, m2.third);

    My m4{};
    m4 = std::move(m3);
    EXPECT_EQ(m3.validMask(), My::FieldSet{});
    EXPECT_TRUE(m3.first.empty()); // moved away

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
    EXPECT_EQ(d.map[My::Zeroth], My::Diff::Type::Unchanged);
    EXPECT_EQ(d.map[My::First], My::Diff::Type::Unchanged);
    EXPECT_EQ(d.map[My::Second], My::Diff::Type::Unchanged);
    EXPECT_EQ(d.map[My::Third], My::Diff::Type::Unchanged);

    // diff with empty
    My m2;
    d = m2.diff(m1);
    EXPECT_EQ(d.differences, 4);
    EXPECT_EQ(d.map[My::Zeroth], My::Diff::Type::Added);
    EXPECT_EQ(d.map[My::First], My::Diff::Type::Added);
    EXPECT_EQ(d.map[My::Second], My::Diff::Type::Added);
    EXPECT_EQ(d.map[My::Third], My::Diff::Type::Added);

    d = m1.diff(m2);
    EXPECT_EQ(d.differences, 4);
    EXPECT_EQ(d.map[My::Zeroth], My::Diff::Type::Removed);
    EXPECT_EQ(d.map[My::First], My::Diff::Type::Removed);
    EXPECT_EQ(d.map[My::Second], My::Diff::Type::Removed);
    EXPECT_EQ(d.map[My::Third], My::Diff::Type::Removed);

    My m3(m1);
    ErSet(My, First, m1, first, "Hello!");
    ErSet(My, Second, m1, second, 12345);
    EXPECT_NE(m3.hash(), m1.hash());
    d = m1.diff(m3);
    EXPECT_EQ(d.differences, 2);
    EXPECT_EQ(d.map[My::Zeroth], My::Diff::Type::Unchanged);
    EXPECT_EQ(d.map[My::First], My::Diff::Type::Changed);
    EXPECT_EQ(d.map[My::Second], My::Diff::Type::Changed);
    EXPECT_EQ(d.map[My::Third], My::Diff::Type::Unchanged);
}

TEST(Reflectable, update)
{
    My m1;
    ErSet(My, Zeroth, m1, zeroth, 34);
    ErSet(My, First, m1, first, "Bye?");
    ErSet(My, Second, m1, second, -54321);
    ErSet(My, Third, m1, third, 9.99);
    auto valid = m1.validMask();
    auto hash = m1.hash();

    // update from self
    auto d = m1.update(m1);
    EXPECT_EQ(d.differences, 0);
    EXPECT_EQ(d.map[My::Zeroth], My::Diff::Type::Unchanged);
    EXPECT_EQ(d.map[My::First], My::Diff::Type::Unchanged);
    EXPECT_EQ(d.map[My::Second], My::Diff::Type::Unchanged);
    EXPECT_EQ(d.map[My::Third], My::Diff::Type::Unchanged);
    
    EXPECT_EQ(m1.zeroth, 34);
    EXPECT_STREQ(m1.first.c_str(), "Bye?");
    EXPECT_EQ(m1.second, -54321);
    EXPECT_DOUBLE_EQ(m1.third, 9.99);

    EXPECT_EQ(valid, m1.validMask());
    EXPECT_EQ(hash, m1.hash());
    
    // update empty
    My m2;
    d = m2.update(m1);
    EXPECT_EQ(d.differences, 4);
    EXPECT_EQ(d.map[My::Zeroth], My::Diff::Type::Added);
    EXPECT_EQ(d.map[My::First], My::Diff::Type::Added);
    EXPECT_EQ(d.map[My::Second], My::Diff::Type::Added);
    EXPECT_EQ(d.map[My::Third], My::Diff::Type::Added);

    EXPECT_EQ(m2.zeroth, 34);
    EXPECT_STREQ(m2.first.c_str(), "Bye?");
    EXPECT_EQ(m2.second, -54321);
    EXPECT_DOUBLE_EQ(m2.third, 9.99);

    EXPECT_EQ(valid, m2.validMask());
    EXPECT_EQ(hash, m2.hash());

    // update from empty
    d = m2.update(My{});
    EXPECT_EQ(d.map[My::Zeroth], My::Diff::Type::Removed);
    EXPECT_EQ(d.map[My::First], My::Diff::Type::Removed);
    EXPECT_EQ(d.map[My::Second], My::Diff::Type::Removed);
    EXPECT_EQ(d.map[My::Third], My::Diff::Type::Removed);
    EXPECT_EQ(m2.validMask(), My::FieldSet{});

    // update from changed
    My m3(m1);
    ErSet(My, First, m3, first, "Hello!");
    ErSet(My, Second, m3, second, 12345);
    
    d = m1.update(m3);
    EXPECT_EQ(d.differences, 2);
    EXPECT_EQ(d.map[My::Zeroth], My::Diff::Type::Unchanged);
    EXPECT_EQ(d.map[My::First], My::Diff::Type::Changed);
    EXPECT_EQ(d.map[My::Second], My::Diff::Type::Changed);
    EXPECT_EQ(d.map[My::Third], My::Diff::Type::Unchanged);

    EXPECT_EQ(m1.zeroth, 34);
    EXPECT_STREQ(m1.first.c_str(), "Hello!");
    EXPECT_EQ(m1.second, 12345);
    EXPECT_DOUBLE_EQ(m1.third, 9.99);

    EXPECT_EQ(m3.validMask(), m1.validMask());
    EXPECT_EQ(m3.hash(), m1.hash());
}

TEST(Reflectable, update_move)
{
    My m1;
    ErSet(My, Zeroth, m1, zeroth, 34);
    ErSet(My, First, m1, first, "Bye?");
    ErSet(My, Second, m1, second, -54321);
    ErSet(My, Third, m1, third, 9.99);
    auto valid = m1.validMask();
    auto hash = m1.hash();

    // update from self
    auto d = m1.update(std::move(m1));
    EXPECT_EQ(d.differences, 0);
    EXPECT_EQ(d.map[My::Zeroth], My::Diff::Type::Unchanged);
    EXPECT_EQ(d.map[My::First], My::Diff::Type::Unchanged);
    EXPECT_EQ(d.map[My::Second], My::Diff::Type::Unchanged);
    EXPECT_EQ(d.map[My::Third], My::Diff::Type::Unchanged);

    EXPECT_EQ(m1.zeroth, 34);
    EXPECT_STREQ(m1.first.c_str(), "Bye?");
    EXPECT_EQ(m1.second, -54321);
    EXPECT_DOUBLE_EQ(m1.third, 9.99);

    EXPECT_EQ(valid, m1.validMask());
    EXPECT_EQ(hash, m1.hash());

    // update empty
    My m1_copy(m1);
    My m2;
    d = m2.update(std::move(m1_copy));
    EXPECT_EQ(d.differences, 4);
    EXPECT_EQ(d.map[My::Zeroth], My::Diff::Type::Added);
    EXPECT_EQ(d.map[My::First], My::Diff::Type::Added);
    EXPECT_EQ(d.map[My::Second], My::Diff::Type::Added);
    EXPECT_EQ(d.map[My::Third], My::Diff::Type::Added);

    EXPECT_EQ(m2.zeroth, 34);
    EXPECT_STREQ(m2.first.c_str(), "Bye?");
    EXPECT_EQ(m2.second, -54321);
    EXPECT_DOUBLE_EQ(m2.third, 9.99);

    EXPECT_EQ(valid, m2.validMask());
    EXPECT_EQ(hash, m2.hash());

    EXPECT_EQ(m1_copy.validMask(), My::FieldSet{});
    EXPECT_TRUE(m1_copy.first.empty()); // moved away
    EXPECT_NE(m1_copy.hash(), hash);

    // update from empty
    My empty{};
    d = m2.update(std::move(empty));
    EXPECT_EQ(d.map[My::Zeroth], My::Diff::Type::Removed);
    EXPECT_EQ(d.map[My::First], My::Diff::Type::Removed);
    EXPECT_EQ(d.map[My::Second], My::Diff::Type::Removed);
    EXPECT_EQ(d.map[My::Third], My::Diff::Type::Removed);
    EXPECT_EQ(m2.validMask(), My::FieldSet{});
    EXPECT_EQ(empty.validMask(), My::FieldSet{});

    // update from changed
    My m3(m1);
    ErSet(My, First, m3, first, "Hello!");
    ErSet(My, Second, m3, second, 12345);
    auto h3 = m3.hash();
 
    d = m1.update(std::move(m3));
    EXPECT_EQ(d.differences, 2);
    EXPECT_EQ(d.map[My::Zeroth], My::Diff::Type::Unchanged);
    EXPECT_EQ(d.map[My::First], My::Diff::Type::Changed);
    EXPECT_EQ(d.map[My::Second], My::Diff::Type::Changed);
    EXPECT_EQ(d.map[My::Third], My::Diff::Type::Unchanged);

    EXPECT_EQ(m1.zeroth, 34);
    EXPECT_STREQ(m1.first.c_str(), "Hello!");
    EXPECT_EQ(m1.second, 12345);
    EXPECT_DOUBLE_EQ(m1.third, 9.99);

    EXPECT_EQ(m1.hash(), h3);

    EXPECT_EQ(m3.validMask(), My::FieldSet{});
    EXPECT_TRUE(m3.first.empty()); // moved away
}

struct Rich
    : public Reflectable<Rich, 7>
{
    enum Field : FieldId
    {
        Default,
        Pointer,
        Flags,
        Time,
        Duration,
        Percent,
        Size
    };

    std::string def;
    std::uint64_t pointer;
    std::uint32_t flags;
    Er::Time::ValueType time;
    Er::Time::ValueType dura;
    double percent;
    std::size_t size;

    ER_REFLECTABLE_FILEDS_BEGIN(Rich)
        ER_REFLECTABLE_FIELD(Rich, Default, Semantics::Default, def),
        ER_REFLECTABLE_FIELD(Rich, Pointer, Semantics::Pointer, pointer),
        ER_REFLECTABLE_FIELD(Rich, Flags, Semantics::Flags, flags),
        ER_REFLECTABLE_FIELD(Rich, Time, Semantics::AbsoluteTime, time),
        ER_REFLECTABLE_FIELD(Rich, Duration, Semantics::Duration, dura),
        ER_REFLECTABLE_FIELD(Rich, Percent, Semantics::Percent, percent),
        ER_REFLECTABLE_FIELD(Rich, Size, Semantics::Size, size)
    ER_REFLECTABLE_FILEDS_END()
};

TEST(Reflectable, format)
{
    Rich r;
    r.def = "Some dumb text";
    r.setValid(Rich::Default);

    r.pointer = std::bit_cast<std::uint64_t>(&r);
    r.setValid(Rich::Pointer);

    r.flags = 0xc0030f31;
    r.setValid(Rich::Flags);

    r.time = Er::Time::now();
    r.setValid(Rich::Time);

    r.dura = 1234567890ULL;
    r.setValid(Rich::Duration);

    r.percent = 19.957;
    r.setValid(Rich::Percent);

    r.size = 1024 * 1024 * 5 + 445;
    r.setValid(Rich::Size);


    for (FieldId id = 0; id < Rich::FieldCount; ++id)
    {
        auto name = r.name(id);
        auto val = r.format(id);

        ErLogInfo("{} = [{}]", name, val);
    }
}