#include "common.hpp"

#include <erebus/rtl/property_bag.hxx>

using namespace Er;


TEST(PropertyBag, visit)
{
    PropertyBag bag;
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
        bool vector_visited = false;

        PropertyBag bag;
        PropertyMap m1;

        bool operator()(const Property& prop, const Empty& v)
        {
            empty_visited = prop.name().empty();
            addProperty(bag, {});
            return true;
        }

        bool operator()(const Property& prop, const Bool& v)
        {
            bool_visited = (prop.name() == Property::Name("test/bool")) && (v == True);
            addProperty(bag, { prop.name(), v });
            return true;
        }

        bool operator()(const Property& prop, const std::int32_t& v)
        {
            int32_visited = (prop.name() == Property::Name("test/int32")) && (v == -12);
            addProperty(bag, { prop.name(), v });
            return true;
        }

        bool operator()(const Property& prop, const std::uint32_t& v)
        {
            uint32_visited = (prop.name() == Property::Name("test/uint32")) && (v == 13);
            addProperty(bag, { prop.name(), v });
            return true;
        }

        bool operator()(const Property& prop, const std::int64_t& v)
        {
            int64_visited = (prop.name() == Property::Name("test/int64")) && (v == -125);
            addProperty(bag, { prop.name(), v });
            return true;
        }

        bool operator()(const Property& prop, const std::uint64_t& v)
        {
            uint64_visited = (prop.name() == Property::Name("test/uint64")) && (v == 555);
            addProperty(bag, { prop.name(), v });
            return true;
        }

        bool operator()(const Property& prop, const double& v)
        {
            double_visited = (prop.name() == Property::Name("test/double")) && (v == -0.1);
            addProperty(bag, { prop.name(), v });
            return true;
        }

        bool operator()(const Property& prop, const std::string& v)
        {
            string_visited = (prop.name() == Property::Name("test/string")) && (v == std::string("xa xa xa"));
            addProperty(bag, { prop.name(), v });
            return true;
        }

        bool operator()(const Property& prop, const Binary& v)
        {
            binary_visited = (prop.name() == Property::Name("test/binary")) && (v == Binary(std::string("xo xo xo")));
            addProperty(bag, { prop.name(), v });
            return true;
        }

        bool operator()(const Property& prop, const PropertyMap& v)
        {
            map_visited = (prop.name() == Property::Name("test/map"));

            for (auto& p : v)
            {
                addProperty(m1, p.second);
            }

            addProperty(bag, { prop.name(), v });
            return true;
        }

        bool operator()(const Property& prop, const PropertyVector& v)
        {
            vector_visited = (prop.name() == Property::Name("test/vector"));

            addProperty(bag, { prop.name(), v });
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


TEST(PropertyBag, find)
{
    PropertyBag bag;
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
    EXPECT_TRUE(p->type() == Property::Type::Empty);

    p = findProperty(bag, "test/bool");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property::Type::Bool);
    EXPECT_EQ(*p->getBool(), True);

    p = findProperty(bag, "test/int32");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property::Type::Int32);
    EXPECT_EQ(*p->getInt32(), -12);

    p = findProperty(bag, "test/uint32");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property::Type::UInt32);
    EXPECT_EQ(*p->getUInt32(), 13);

    p = findProperty(bag, "test/int64");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property::Type::Int64);
    EXPECT_EQ(*p->getInt64(), -125);

    p = findProperty(bag, "test/uint64");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property::Type::UInt64);
    EXPECT_EQ(*p->getUInt64(), 555);

    p = findProperty(bag, "test/double");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property::Type::Double);
    EXPECT_DOUBLE_EQ(*p->getDouble(), -0.1);

    p = findProperty(bag, "test/string");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property::Type::String);
    EXPECT_EQ(*p->getString(), std::string("xa xa xa"));

    p = findProperty(bag, "test/binary");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property::Type::Binary);
    EXPECT_EQ(*p->getBinary(), Binary(std::string("xo xo xo")));

    p = findProperty(bag, "test/map");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property::Type::Map);
    EXPECT_TRUE(*p->getMap() == m1);
}

TEST(PropertyBag, findPropertyByPathMap)
{
    PropertyMap top;
    {
        addProperty(top, { "0bool", true });
        addProperty(top, { "0int32", std::int32_t(-12) });
        addProperty(top, { "0string", "string 0" });

        {
            PropertyVector v;

            addProperty(v, { "1int32", std::int32_t(11) });

            {
                PropertyMap m;

                addProperty(m, { "2int32", std::int32_t(-6) });
                addProperty(m, { "2string", "string 2" });

                addProperty(v, { "1map", std::move(m) });
            }

            addProperty(v, { "1double", -9.9 });

            addProperty(top, { "0vector", std::move(v) });
        }
    }

    // empty path
    auto prop = findPropertyByPath(top, "");
    EXPECT_EQ(prop, nullptr);

    // top-level item
    prop = findPropertyByPath(top, "0bool", '/', Property::Type::Bool);
    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->type(), Property::Type::Bool);
    EXPECT_EQ(*prop->getBool(), true);

    // wrong type
    prop = findPropertyByPath(top, "0bool", '/', Property::Type::UInt32);
    EXPECT_EQ(prop, nullptr);

    // wrong name
    prop = findPropertyByPath(top, "2bool", '/', Property::Type::Bool);
    EXPECT_EQ(prop, nullptr);

    prop = findPropertyByPath(top, "0int32", '/', Property::Type::Int32);
    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->type(), Property::Type::Int32);
    EXPECT_EQ(*prop->getInt32(), -12);

    prop = findPropertyByPath(top, "0string", '/', Property::Type::String);
    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->type(), Property::Type::String);
    EXPECT_STREQ(prop->getString()->c_str(), "string 0");

    prop = findPropertyByPath(top, "0vector", '/', Property::Type::Vector);
    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->type(), Property::Type::Vector);
    EXPECT_EQ(prop->getVector()->size(), 3);

    // level 1
    prop = findPropertyByPath(top, "0vector/1int32", '/', Property::Type::Int32);
    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->type(), Property::Type::Int32);
    EXPECT_EQ(*prop->getInt32(), 11);

    prop = findPropertyByPath(top, "0vector/1double", '/', Property::Type::Double);
    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->type(), Property::Type::Double);
    EXPECT_DOUBLE_EQ(*prop->getDouble(), -9.9);

    prop = findPropertyByPath(top, "0vector/1map", '/', Property::Type::Map);
    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->type(), Property::Type::Map);
    EXPECT_EQ(prop->getMap()->size(), 2);

    // level 2
    prop = findPropertyByPath(top, "0vector/1map/2int32", '/', Property::Type::Int32);
    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->type(), Property::Type::Int32);
    EXPECT_EQ(*prop->getInt32(), -6);

    prop = findPropertyByPath(top, "0vector/1map/2string", '/', Property::Type::String);
    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->type(), Property::Type::String);
    EXPECT_STREQ(prop->getString()->c_str(), "string 2");

    // wrong paths
    EXPECT_EQ(nullptr, findPropertyByPath(top, "0vector/1map/3string", '/', Property::Type::String));
    EXPECT_EQ(nullptr, findPropertyByPath(top, "0vector/1map/2string", '.', Property::Type::String));
    EXPECT_EQ(nullptr, findPropertyByPath(top, "0vector/1map\\2string", '/', Property::Type::String));
    EXPECT_EQ(nullptr, findPropertyByPath(top, "0vector//2string", '/', Property::Type::String));
    EXPECT_EQ(nullptr, findPropertyByPath(top, "0vector/1map/", '/', Property::Type::String));
    EXPECT_EQ(nullptr, findPropertyByPath(top, "/2string", '/', Property::Type::String));
    EXPECT_EQ(nullptr, findPropertyByPath(top, "//", '/', Property::Type::String));
    EXPECT_EQ(nullptr, findPropertyByPath(top, "/", '/', Property::Type::String));
}

TEST(PropertyBag, findPropertyByPathVector)
{
    PropertyVector top;
    {
        addProperty(top, { "0bool", true });
        addProperty(top, { "0int32", std::int32_t(-12) });
        addProperty(top, { "0string", "string 0" });

        {
            PropertyVector v;

            addProperty(v, { "1int32", std::int32_t(11) });

            {
                PropertyMap m;

                addProperty(m, { "2int32", std::int32_t(-6) });
                addProperty(m, { "2string", "string 2" });

                addProperty(v, { "1map", std::move(m) });
            }

            addProperty(v, { "1double", -9.9 });

            addProperty(top, { "0vector", std::move(v) });
        }
    }

    // empty path
    auto prop = findPropertyByPath(top, "");
    EXPECT_EQ(prop, nullptr);

    // top-level item
    prop = findPropertyByPath(top, "0bool", '/', Property::Type::Bool);
    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->type(), Property::Type::Bool);
    EXPECT_EQ(*prop->getBool(), true);

    // wrong type
    prop = findPropertyByPath(top, "0bool", '/', Property::Type::UInt32);
    EXPECT_EQ(prop, nullptr);

    // wrong name
    prop = findPropertyByPath(top, "2bool", '/', Property::Type::Bool);
    EXPECT_EQ(prop, nullptr);

    prop = findPropertyByPath(top, "0int32", '/', Property::Type::Int32);
    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->type(), Property::Type::Int32);
    EXPECT_EQ(*prop->getInt32(), -12);

    prop = findPropertyByPath(top, "0string", '/', Property::Type::String);
    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->type(), Property::Type::String);
    EXPECT_STREQ(prop->getString()->c_str(), "string 0");

    prop = findPropertyByPath(top, "0vector", '/', Property::Type::Vector);
    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->type(), Property::Type::Vector);
    EXPECT_EQ(prop->getVector()->size(), 3);

    // level 1
    prop = findPropertyByPath(top, "0vector/1int32", '/', Property::Type::Int32);
    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->type(), Property::Type::Int32);
    EXPECT_EQ(*prop->getInt32(), 11);

    prop = findPropertyByPath(top, "0vector/1double", '/', Property::Type::Double);
    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->type(), Property::Type::Double);
    EXPECT_DOUBLE_EQ(*prop->getDouble(), -9.9);

    prop = findPropertyByPath(top, "0vector/1map", '/', Property::Type::Map);
    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->type(), Property::Type::Map);
    EXPECT_EQ(prop->getMap()->size(), 2);

    // level 2
    prop = findPropertyByPath(top, "0vector/1map/2int32", '/', Property::Type::Int32);
    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->type(), Property::Type::Int32);
    EXPECT_EQ(*prop->getInt32(), -6);

    prop = findPropertyByPath(top, "0vector/1map/2string", '/', Property::Type::String);
    ASSERT_NE(prop, nullptr);
    ASSERT_EQ(prop->type(), Property::Type::String);
    EXPECT_STREQ(prop->getString()->c_str(), "string 2");

    // wrong paths
    EXPECT_EQ(nullptr, findPropertyByPath(top, "0vector/1map/3string", '/', Property::Type::String));
    EXPECT_EQ(nullptr, findPropertyByPath(top, "0vector/1map/2string", '.', Property::Type::String));
    EXPECT_EQ(nullptr, findPropertyByPath(top, "0vector/1map\\2string", '/', Property::Type::String));
    EXPECT_EQ(nullptr, findPropertyByPath(top, "0vector//2string", '/', Property::Type::String));
    EXPECT_EQ(nullptr, findPropertyByPath(top, "0vector/1map/", '/', Property::Type::String));
    EXPECT_EQ(nullptr, findPropertyByPath(top, "/2string", '/', Property::Type::String));
    EXPECT_EQ(nullptr, findPropertyByPath(top, "//", '/', Property::Type::String));
    EXPECT_EQ(nullptr, findPropertyByPath(top, "/", '/', Property::Type::String));
}

TEST(PropertyBag, loadJson)
{
const std::string_view kSource =
R"(
{
    "bool0":true,
    "int0":-123456,
    "uint0":98765,
    "double0":-0.9,
    "string0":"top level string",
    "array0":[
        { 
            "string1_0":"level 1 string 0",  
            "bool1_0":false 
        },
        { 
            "string1_1":"level 1 string 1",  
            "bool1_1":true 
        }
    ],
    "object0":{
        "array1_0":[
            10, 20, 30, 40, 50
        ],
        "int1_0":555,
        "string1_0":"level 1 string 2"
    }
}
)";

    auto prop = loadJson(kSource);

    ASSERT_EQ(prop.type(), Er::Property::Type::Map);

    auto m = prop.getMap();
    EXPECT_EQ(m->size(), 7);
    
    {
        auto it = m->find("bool0");
        ASSERT_NE(it, m->end());
        EXPECT_STREQ(it->first.c_str(), "bool0");
        EXPECT_STREQ(it->second.name().c_str(), "bool0");
        ASSERT_EQ(it->second.type(), Er::Property::Type::Bool);
        EXPECT_EQ(*it->second.getBool(), true);

        it = m->find("int0");
        ASSERT_NE(it, m->end());
        EXPECT_STREQ(it->first.c_str(), "int0");
        EXPECT_STREQ(it->second.name().c_str(), "int0");
        ASSERT_EQ(it->second.type(), Er::Property::Type::Int64);
        EXPECT_EQ(*it->second.getInt64(), -123456);

        it = m->find("uint0");
        ASSERT_NE(it, m->end());
        EXPECT_STREQ(it->first.c_str(), "uint0");
        EXPECT_STREQ(it->second.name().c_str(), "uint0");
        ASSERT_EQ(it->second.type(), Er::Property::Type::Int64); // loaded as Int64
        EXPECT_EQ(*it->second.getInt64(), 98765);

        it = m->find("double0");
        ASSERT_NE(it, m->end());
        EXPECT_STREQ(it->first.c_str(), "double0");
        EXPECT_STREQ(it->second.name().c_str(), "double0");
        ASSERT_EQ(it->second.type(), Er::Property::Type::Double);
        EXPECT_DOUBLE_EQ(*it->second.getDouble(), -0.9);

        it = m->find("string0");
        ASSERT_NE(it, m->end());
        EXPECT_STREQ(it->first.c_str(), "string0");
        EXPECT_STREQ(it->second.name().c_str(), "string0");
        ASSERT_EQ(it->second.type(), Er::Property::Type::String);
        EXPECT_STREQ(it->second.getString()->c_str(), "top level string");

        it = m->find("array0");
        ASSERT_NE(it, m->end());
        EXPECT_STREQ(it->first.c_str(), "array0");
        EXPECT_STREQ(it->second.name().c_str(), "array0");
        ASSERT_EQ(it->second.type(), Er::Property::Type::Vector);
        auto v = it->second.getVector();
        ASSERT_EQ(v->size(), 2);
        {
            auto p = &(*v)[0];

            EXPECT_STREQ(p->name().c_str(), "");
            ASSERT_EQ(p->type(), Er::Property::Type::Map);
            auto m = p->getMap();
            ASSERT_EQ(m->size(), 2);
            {
                auto it = m->find("string1_0");
                ASSERT_NE(it, m->end());
                EXPECT_STREQ(it->first.c_str(), "string1_0");
                EXPECT_STREQ(it->second.name().c_str(), "string1_0");
                ASSERT_EQ(it->second.type(), Er::Property::Type::String);
                EXPECT_STREQ(it->second.getString()->c_str(), "level 1 string 0");

                it = m->find("bool1_0");
                ASSERT_NE(it, m->end());
                EXPECT_STREQ(it->first.c_str(), "bool1_0");
                EXPECT_STREQ(it->second.name().c_str(), "bool1_0");
                ASSERT_EQ(it->second.type(), Er::Property::Type::Bool);
                EXPECT_EQ(*it->second.getBool(), false);
            }

            p = &(*v)[1];

            EXPECT_STREQ(p->name().c_str(), "");
            ASSERT_EQ(p->type(), Er::Property::Type::Map);
            m = p->getMap();
            ASSERT_EQ(m->size(), 2);
            {
                auto it = m->find("string1_1");
                ASSERT_NE(it, m->end());
                EXPECT_STREQ(it->first.c_str(), "string1_1");
                EXPECT_STREQ(it->second.name().c_str(), "string1_1");
                ASSERT_EQ(it->second.type(), Er::Property::Type::String);
                EXPECT_STREQ(it->second.getString()->c_str(), "level 1 string 1");

                it = m->find("bool1_1");
                ASSERT_NE(it, m->end());
                EXPECT_STREQ(it->first.c_str(), "bool1_1");
                EXPECT_STREQ(it->second.name().c_str(), "bool1_1");
                ASSERT_EQ(it->second.type(), Er::Property::Type::Bool);
                EXPECT_EQ(*it->second.getBool(), true);
            }
        }

        it = m->find("object0");
        ASSERT_NE(it, m->end());
        EXPECT_STREQ(it->first.c_str(), "object0");
        EXPECT_STREQ(it->second.name().c_str(), "object0");
        ASSERT_EQ(it->second.type(), Er::Property::Type::Map);
        auto o = it->second.getMap();
        ASSERT_EQ(o->size(), 3);
        {
            auto it = o->find("array1_0");
            ASSERT_NE(it, o->end());
            EXPECT_STREQ(it->first.c_str(), "array1_0");
            EXPECT_STREQ(it->second.name().c_str(), "array1_0");
            ASSERT_EQ(it->second.type(), Er::Property::Type::Vector);
            auto v = it->second.getVector();
            ASSERT_EQ(v->size(), 5);
            {
                auto p = &(*v)[0];
                ASSERT_EQ(p->type(), Er::Property::Type::Int64);
                EXPECT_EQ(*p->getInt64(), 10);

                p = &(*v)[1];
                ASSERT_EQ(p->type(), Er::Property::Type::Int64);
                EXPECT_EQ(*p->getInt64(), 20);

                p = &(*v)[2];
                ASSERT_EQ(p->type(), Er::Property::Type::Int64);
                EXPECT_EQ(*p->getInt64(), 30);

                p = &(*v)[3];
                ASSERT_EQ(p->type(), Er::Property::Type::Int64);
                EXPECT_EQ(*p->getInt64(), 40);

                p = &(*v)[4];
                ASSERT_EQ(p->type(), Er::Property::Type::Int64);
                EXPECT_EQ(*p->getInt64(), 50);
            }

            it = o->find("int1_0");
            ASSERT_NE(it, o->end());
            EXPECT_STREQ(it->first.c_str(), "int1_0");
            EXPECT_STREQ(it->second.name().c_str(), "int1_0");
            ASSERT_EQ(it->second.type(), Er::Property::Type::Int64);
            EXPECT_EQ(*it->second.getInt64(), 555);

            it = o->find("string1_0");
            ASSERT_NE(it, o->end());
            EXPECT_STREQ(it->first.c_str(), "string1_0");
            EXPECT_STREQ(it->second.name().c_str(), "string1_0");
            ASSERT_EQ(it->second.type(), Er::Property::Type::String);
            EXPECT_STREQ(it->second.getString()->c_str(), "level 1 string 2");
        }
    }
}