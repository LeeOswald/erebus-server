#include "common.hpp"

#include <erebus/rtl/reflectable.hxx>

using namespace Er;


struct My;

template <>
struct ReflectableTraits<My>
{
    using SelfType = My;

    struct FieldIds
    {
        enum : unsigned
        {
            First,
            Second,

            _FieldCount
        };
    };
};

struct My
    : public Reflectable<ReflectableTraits<My>>
{
    ReflectableField<FieldIds::First, std::string, "first", Semantics::Default> first;
    ReflectableField<FieldIds::Second, std::int64_t, "second", Semantics::Default> second;

    static Fields const& fields() noexcept
    {
        static const Fields fields = 
        {
            decltype(first)::fieldInfo(&My::first),
            decltype(second)::fieldInfo(&My::second)
        };

        return fields;
    }
};



TEST(Reflectable, base)
{
    auto& flds = My::fields();
    for (auto& f : flds)
    {
        std::cout << "#" << f.id << "." << f.name << "\n";
    }

    std::cout << "-------------------------\n";

    My m1;
    m1.first.value = "first fuck";
    m1.second.value = 123;

    My m2;
    m2.first.value = "second fuck";
    m2.second.value = 123;

    EXPECT_FALSE(flds[My::FieldIds::First].comparator(m1, m2));
    EXPECT_TRUE(flds[My::FieldIds::Second].comparator(m1, m2));
}