#include "common.hpp"

#include <erebus/rtl/property_bag2.hxx>

using namespace Er;


TEST(PropertyBag2, visit)
{
    PropertyBag2 bag;
    addProperty(bag, {});
    addProperty(bag, { "test/bool", True });
    addProperty(bag, { "test/int32", std::int32_t(-12) });
    addProperty(bag, { "test/uint32", std::uint32_t(13) });
    addProperty(bag, { "test/int64", std::int64_t(-125) });
    addProperty(bag, { "test/uint64", std::uint64_t(555) });
    addProperty(bag, { "test/double", -0.1 });
    addProperty(bag, { "test/string", std::string("xa xa xa") });
    addProperty(bag, { "test/binary", Binary(std::string("xo xo xo")) });

    PropertyMap m1;
    addProperty(m1, { "test/int32", std::int32_t(-99) });
    addProperty(m1, { "test/string", std::string("uxa uxa uxa") });
    addProperty(bag, { "test/map", m1 });

    struct Visitor
    {
        bool empty_visited = false;
        bool bool_visited = false;
        bool int32_visited = false;
        bool uint32_visited = false;
        bool int64_visited = false;
        bool uint64_visited = false;
        bool double_visited = false;
        bool string_visited = false;
        bool binary_visited = false;
        bool map_visited = false;

        PropertyBag2 bag;
        PropertyMap m1;

        bool operator()(const Property2::Name& name, const Empty& v)
        {
            empty_visited = name.empty();
            addProperty(bag, {});
            return true;
        }

        bool operator()(const Property2::Name& name, const Bool& v)
        {
            bool_visited = (name == Property2::Name("test/bool")) && (v == True);
            addProperty(bag, { name, v });
            return true;
        }

        bool operator()(const Property2::Name& name, const std::int32_t& v)
        {
            int32_visited = (name == Property2::Name("test/int32")) && (v == -12);
            addProperty(bag, { name, v });
            return true;
        }

        bool operator()(const Property2::Name& name, const std::uint32_t& v)
        {
            uint32_visited = (name == Property2::Name("test/uint32")) && (v == 13);
            addProperty(bag, { name, v });
            return true;
        }

        bool operator()(const Property2::Name& name, const std::int64_t& v)
        {
            int64_visited = (name == Property2::Name("test/int64")) && (v == -125);
            addProperty(bag, { name, v });
            return true;
        }

        bool operator()(const Property2::Name& name, const std::uint64_t& v)
        {
            uint64_visited = (name == Property2::Name("test/uint64")) && (v == 555);
            addProperty(bag, { name, v });
            return true;
        }

        bool operator()(const Property2::Name& name, const double& v)
        {
            double_visited = (name == Property2::Name("test/double")) && (v == -0.1);
            addProperty(bag, { name, v });
            return true;
        }

        bool operator()(const Property2::Name& name, const std::string& v)
        {
            string_visited = (name == Property2::Name("test/string")) && (v == std::string("xa xa xa"));
            addProperty(bag, { name, v });
            return true;
        }

        bool operator()(const Property2::Name& name, const Binary& v)
        {
            binary_visited = (name == Property2::Name("test/binary")) && (v == Binary(std::string("xo xo xo")));
            addProperty(bag, { name, v });
            return true;
        }

        bool operator()(const Property2::Name& name, const PropertyMap& v)
        {
            map_visited = (name == Property2::Name("test/map"));

            for (auto& p : v)
            {
                addProperty(m1, p.second);
            }

            addProperty(bag, { name, v });
            return true;
        }
    };

    Visitor vis;
    visit(bag, vis);

    EXPECT_TRUE(vis.empty_visited);
    EXPECT_TRUE(vis.bool_visited);
    EXPECT_TRUE(vis.int32_visited);
    EXPECT_TRUE(vis.uint32_visited);
    EXPECT_TRUE(vis.int64_visited);
    EXPECT_TRUE(vis.uint64_visited);
    EXPECT_TRUE(vis.double_visited);
    EXPECT_TRUE(vis.string_visited);
    EXPECT_TRUE(vis.binary_visited);
    EXPECT_TRUE(vis.map_visited);

    EXPECT_TRUE(vis.bag == bag);
    EXPECT_TRUE(vis.m1 == m1);
}


TEST(PropertyBag2, find)
{
    PropertyBag2 bag;
    addProperty(bag, {});
    addProperty(bag, { "test/bool", True });
    addProperty(bag, { "test/int32", std::int32_t(-12) });
    addProperty(bag, { "test/uint32", std::uint32_t(13) });
    addProperty(bag, { "test/int64", std::int64_t(-125) });
    addProperty(bag, { "test/uint64", std::uint64_t(555) });
    addProperty(bag, { "test/double", -0.1 });
    addProperty(bag, { "test/string", std::string("xa xa xa") });
    addProperty(bag, { "test/binary", Binary(std::string("xo xo xo")) });

    PropertyMap m1;
    addProperty(m1, { "test/int32", std::int32_t(-99) });
    addProperty(m1, { "test/string", std::string("uxa uxa uxa") });
    addProperty(bag, { "test/map", m1 });

    auto p = findProperty(bag, "");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property2::Type::Empty);

    p = findProperty(bag, "test/bool");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property2::Type::Bool);
    EXPECT_EQ(*p->getBool(), True);

    p = findProperty(bag, "test/int32");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property2::Type::Int32);
    EXPECT_EQ(*p->getInt32(), -12);

    p = findProperty(bag, "test/uint32");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property2::Type::UInt32);
    EXPECT_EQ(*p->getUInt32(), 13);

    p = findProperty(bag, "test/int64");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property2::Type::Int64);
    EXPECT_EQ(*p->getInt64(), -125);

    p = findProperty(bag, "test/uint64");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property2::Type::UInt64);
    EXPECT_EQ(*p->getUInt64(), 555);

    p = findProperty(bag, "test/double");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property2::Type::Double);
    EXPECT_DOUBLE_EQ(*p->getDouble(), -0.1);

    p = findProperty(bag, "test/string");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property2::Type::String);
    EXPECT_EQ(*p->getString(), std::string("xa xa xa"));

    p = findProperty(bag, "test/binary");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property2::Type::Binary);
    EXPECT_EQ(*p->getBinary(), Binary(std::string("xo xo xo")));

    p = findProperty(bag, "test/map");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property2::Type::Map);
    EXPECT_TRUE(*p->getMap() == m1);
}