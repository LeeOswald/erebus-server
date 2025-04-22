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
        ER_REFLECTABLE_FIELD(My, Zeroth, std::int32_t, Semantics::Default, zeroth),
        ER_REFLECTABLE_FIELD(My, First, std::string, Semantics::Default, first),
        ER_REFLECTABLE_FIELD(My, Second, std::int64_t, Semantics::Default, second),
        ER_REFLECTABLE_FIELD(My, Third, double, Semantics::Default, third)
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
    
    m1.first = "first fuck";
    m1.setValid(My::FieldIds::First); 
    
    m1.second = 123;
    m1.setValid(My::FieldIds::Second);

    My m2;
    
    m2.first = "second fuck";
    m2.setValid(My::FieldIds::First);
    
    m2.second = 123;
    m2.setValid(My::FieldIds::Second);

    EXPECT_FALSE(flds[My::FieldIds::First].comparator(m1, m2));
    EXPECT_TRUE(flds[My::FieldIds::Second].comparator(m1, m2));

    auto h1 = m1.updateHash();
    auto h2 = m2.updateHash();
    EXPECT_NE(h1, h2);

    My m3;

    m3.first = "second fuck";
    m3.setValid(My::FieldIds::First);

    m3.second = 123;
    m3.setValid(My::FieldIds::Second);

    auto h3 = m3.updateHash();
    EXPECT_EQ(h2, h3);
}