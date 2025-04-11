#include "common.hpp"

#include <erebus/rtl/property2.hxx>

using namespace Er;


TEST(Property2, Construction)
{
    // Empty
    {
        Property2 v;

        EXPECT_EQ(v.type(), Property2::Type::Empty);
        EXPECT_TRUE(v.empty());
        EXPECT_FALSE(!!v.getBool());
        EXPECT_TRUE(v.name().empty());
    }

    // Bool
    {
        Property2 v1("test/bool", True);
        EXPECT_EQ(v1.type(), Property2::Type::Bool);
        EXPECT_FALSE(v1.empty());
        ASSERT_TRUE(!!v1.getBool());
        EXPECT_EQ(*v1.getBool(), True);
        EXPECT_EQ(v1.name(), std::string_view("test/bool"));

        Property2 v2("test/bool", true);
        EXPECT_EQ(v2.type(), Property2::Type::Bool);
        EXPECT_FALSE(v2.empty());
        ASSERT_TRUE(!!v2.getBool());
        EXPECT_EQ(*v2.getBool(), True);
        EXPECT_EQ(v2.name(), std::string_view("test/bool"));

        Property2 v3("test/bool", False);
        EXPECT_EQ(v3.type(), Property2::Type::Bool);
        EXPECT_FALSE(v3.empty());
        ASSERT_TRUE(!!v3.getBool());
        EXPECT_EQ(*v3.getBool(), False);
        EXPECT_EQ(v3.name(), std::string_view("test/bool"));

        Property2 v4("test/bool", false);
        EXPECT_EQ(v4.type(), Property2::Type::Bool);
        EXPECT_FALSE(v4.empty());
        ASSERT_TRUE(!!v4.getBool());
        EXPECT_EQ(*v4.getBool(), False);
        EXPECT_EQ(v4.name(), std::string_view("test/bool"));
    }

    // Int32
    {
        Property2 v1("test/int32", std::int32_t(-12));
        EXPECT_EQ(v1.type(), Property2::Type::Int32);
        EXPECT_FALSE(v1.empty());
        ASSERT_TRUE(!!v1.getInt32());
        EXPECT_EQ(*v1.getInt32(), -12);
        EXPECT_EQ(v1.name(), std::string_view("test/int32"));
    }

    // UInt32
    {
        Property2 v1("test/uint32", std::uint32_t(0xf0000001));
        EXPECT_EQ(v1.type(), Property2::Type::UInt32);
        EXPECT_FALSE(v1.empty());
        ASSERT_TRUE(!!v1.getUInt32());
        EXPECT_EQ(*v1.getUInt32(), 0xf0000001);
        EXPECT_EQ(v1.name(), std::string_view("test/uint32"));
    }

    // Int64
    {
        Property2 v1("test/int64", std::int64_t(-9223372036854775803LL));
        EXPECT_EQ(v1.type(), Property2::Type::Int64);
        EXPECT_FALSE(v1.empty());
        ASSERT_TRUE(!!v1.getInt64());
        EXPECT_EQ(*v1.getInt64(), -9223372036854775803LL);
        EXPECT_EQ(v1.name(), std::string_view("test/int64"));
    }

    // UInt64
    {
        Property2 v1("test/uint64", std::uint64_t(0x8000000000000005ULL));
        EXPECT_EQ(v1.type(), Property2::Type::UInt64);
        EXPECT_FALSE(v1.empty());
        ASSERT_TRUE(!!v1.getUInt64());
        EXPECT_EQ(*v1.getUInt64(), 0x8000000000000005ULL);
        EXPECT_EQ(v1.name(), std::string_view("test/uint64"));
    }

    // Double
    {
        Property2 v1("test/double", -0.1);
        EXPECT_EQ(v1.type(), Property2::Type::Double);
        EXPECT_FALSE(v1.empty());
        ASSERT_TRUE(!!v1.getDouble());
        EXPECT_DOUBLE_EQ(*v1.getDouble(), -0.1);
        EXPECT_EQ(v1.name(), std::string_view("test/double"));
    }

    // String
    {
        Property2 v1("test/string", "from_char*");
        EXPECT_EQ(v1.type(), Property2::Type::String);
        EXPECT_FALSE(v1.empty());
        ASSERT_TRUE(!!v1.getString());
        EXPECT_EQ(*v1.getString(), std::string_view("from_char*"));
        EXPECT_EQ(v1.name(), std::string_view("test/string"));

        Property2 v2("test/string", std::string_view("from_std::string_view"));
        EXPECT_EQ(v2.type(), Property2::Type::String);
        EXPECT_FALSE(v2.empty());
        ASSERT_TRUE(!!v2.getString());
        EXPECT_EQ(*v2.getString(), std::string_view("from_std::string_view"));
        EXPECT_EQ(v2.name(), std::string_view("test/string"));

        const std::string s("from_const std::string&");
        Property2 v3("test/string", s);
        EXPECT_EQ(v3.type(), Property2::Type::String);
        EXPECT_FALSE(v3.empty());
        ASSERT_TRUE(!!v3.getString());
        EXPECT_EQ(*v3.getString(), s);
        EXPECT_EQ(v3.name(), std::string_view("test/string"));

        Property2 v4("test/string", std::string("from_std::string&&"));
        EXPECT_EQ(v4.type(), Property2::Type::String);
        EXPECT_FALSE(v4.empty());
        ASSERT_TRUE(!!v4.getString());
        EXPECT_EQ(*v4.getString(), std::string_view("from_std::string&&"));
        EXPECT_EQ(v4.name(), std::string_view("test/string"));
    }

    // Binary
    {
        const Binary s("from_const Binary&");
        Property2 v3("test/binary", s);
        EXPECT_EQ(v3.type(), Property2::Type::Binary);
        EXPECT_FALSE(v3.empty());
        ASSERT_TRUE(!!v3.getBinary());
        EXPECT_EQ(*v3.getBinary(), s);
        EXPECT_EQ(v3.name(), std::string_view("test/binary"));

        Property2 v4("test/binary", Binary(std::string("from_Binary&&")));
        EXPECT_EQ(v4.type(), Property2::Type::Binary);
        EXPECT_FALSE(v4.empty());
        ASSERT_TRUE(!!v4.getBinary());
        EXPECT_EQ(*v4.getBinary(), Binary(std::string("from_Binary&&")));
        EXPECT_EQ(v4.name(), std::string_view("test/binary"));
    }

    // Map
    {
        Property2 v;
        Property2 v1("test/bool", True);
        Property2 v2("test/int32", std::int32_t(-12));

        Property2::Map m;
        addProperty(m, v);
        addProperty(m, v1);
        addProperty(m, v2);
        
        // from Map const&
        Property2 p1("test/map", m);

        EXPECT_EQ(p1.type(), Property2::Type::Map);
        EXPECT_FALSE(p1.empty());
        ASSERT_TRUE(!!p1.getMap());
        EXPECT_EQ(p1.name(), std::string_view("test/map"));
        auto& rm = *p1.getMap();
        ASSERT_EQ(rm.size(), 3);

        auto it = rm.find("");
        ASSERT_NE(it, rm.end());

        it = rm.find("test/bool");
        ASSERT_NE(it, rm.end());
        ASSERT_TRUE(!!it->second.getBool());
        EXPECT_EQ(*it->second.getBool(), True);

        it = rm.find("test/int32");
        ASSERT_NE(it, rm.end());
        ASSERT_TRUE(!!it->second.getInt32());
        EXPECT_EQ(*it->second.getInt32(), -12);

        // from Map&&
        Property2::Map m2(m);
        
        Property2 p2("test/map", std::move(m2));

        EXPECT_EQ(p2.type(), Property2::Type::Map);
        EXPECT_FALSE(p2.empty());
        ASSERT_TRUE(!!p2.getMap());
        EXPECT_EQ(p2.name(), std::string_view("test/map"));
        auto& rm2 = *p2.getMap();
        ASSERT_EQ(rm2.size(), 3);

        it = rm2.find("");
        ASSERT_NE(it, rm2.end());

        it = rm2.find("test/bool");
        ASSERT_NE(it, rm2.end());
        ASSERT_TRUE(!!it->second.getBool());
        EXPECT_EQ(*it->second.getBool(), True);

        it = rm2.find("test/int32");
        ASSERT_NE(it, rm2.end());
        ASSERT_TRUE(!!it->second.getInt32());
        EXPECT_EQ(*it->second.getInt32(), -12);

    }

    ErLogInfo("sizeof(Property2) = {}", sizeof(Property2));
    ErLogInfo("sizeof(Property2::Map) = {}", sizeof(Property2::Map));
    ErLogInfo("sizeof(std::string) = {}", sizeof(std::string));
}

TEST(Property2, Copying)
{
    {
        Property2 v;
        
        Property2 v1(v);
        EXPECT_TRUE(v1.empty());
        EXPECT_FALSE(!!v1.getBool());
        EXPECT_TRUE(v1.name().empty());

        Property2 v2("test/uint32", std::uint32_t(12));
        v2 = v;
        EXPECT_TRUE(v2.empty());
        EXPECT_FALSE(!!v2.getBool());
        EXPECT_TRUE(v2.name().empty());
    }

    {
        Property2 v("test/int32", std::int32_t(-12));
        
        Property2 v1(v);
        EXPECT_EQ(v1.type(), Property2::Type::Int32);
        EXPECT_FALSE(v1.empty());
        ASSERT_TRUE(!!v1.getInt32());
        EXPECT_EQ(*v1.getInt32(), -12);
        EXPECT_EQ(v1.name(), std::string_view("test/int32"));

        Property2 v2("test/uint32", std::uint32_t(12));
        v2 = v;
        EXPECT_EQ(v2.type(), Property2::Type::Int32);
        EXPECT_FALSE(v2.empty());
        ASSERT_TRUE(!!v2.getInt32());
        EXPECT_EQ(*v2.getInt32(), -12);
        EXPECT_EQ(v2.name(), std::string_view("test/int32"));
    }

    {
        Property2 v("test/uint32", std::uint32_t(0xf0000001));
        Property2 v1(v);
        EXPECT_EQ(v1.type(), Property2::Type::UInt32);
        EXPECT_FALSE(v1.empty());
        ASSERT_TRUE(!!v1.getUInt32());
        EXPECT_EQ(*v1.getUInt32(), 0xf0000001);
        EXPECT_EQ(v1.name(), std::string_view("test/uint32"));

        Property2 v2("test/uint32", std::uint32_t(12));
        v2 = v;
        EXPECT_EQ(v2.type(), Property2::Type::UInt32);
        EXPECT_FALSE(v2.empty());
        ASSERT_TRUE(!!v2.getUInt32());
        EXPECT_EQ(*v2.getUInt32(), 0xf0000001);
        EXPECT_EQ(v2.name(), std::string_view("test/uint32"));
    }

    // Int64
    {
        Property2 v("test/int64", std::int64_t(-9223372036854775803LL));
        Property2 v1(v);
        EXPECT_EQ(v1.type(), Property2::Type::Int64);
        EXPECT_FALSE(v1.empty());
        ASSERT_TRUE(!!v1.getInt64());
        EXPECT_EQ(*v1.getInt64(), -9223372036854775803LL);
        EXPECT_EQ(v1.name(), std::string_view("test/int64"));

        Property2 v2("test/uint32", std::uint32_t(12));
        v2 = v;
        EXPECT_EQ(v2.type(), Property2::Type::Int64);
        EXPECT_FALSE(v2.empty());
        ASSERT_TRUE(!!v2.getInt64());
        EXPECT_EQ(*v2.getInt64(), -9223372036854775803LL);
        EXPECT_EQ(v2.name(), std::string_view("test/int64"));
    }

    // UInt64
    {
        Property2 v("test/uint64", std::uint64_t(0x8000000000000005ULL));
        Property2 v1(v);
        EXPECT_EQ(v1.type(), Property2::Type::UInt64);
        EXPECT_FALSE(v1.empty());
        ASSERT_TRUE(!!v1.getUInt64());
        EXPECT_EQ(*v1.getUInt64(), 0x8000000000000005ULL);
        EXPECT_EQ(v1.name(), std::string_view("test/uint64"));

        Property2 v2("test/uint32", std::uint32_t(12));
        v2 = v;
        EXPECT_EQ(v2.type(), Property2::Type::UInt64);
        EXPECT_FALSE(v2.empty());
        ASSERT_TRUE(!!v2.getUInt64());
        EXPECT_EQ(*v2.getUInt64(), 0x8000000000000005ULL);
        EXPECT_EQ(v2.name(), std::string_view("test/uint64"));
    }

    // Double
    {
        Property2 v("test/double", -0.1);
        Property2 v1(v);
        EXPECT_EQ(v1.type(), Property2::Type::Double);
        EXPECT_FALSE(v1.empty());
        ASSERT_TRUE(!!v1.getDouble());
        EXPECT_DOUBLE_EQ(*v1.getDouble(), -0.1);
        EXPECT_EQ(v1.name(), std::string_view("test/double"));

        Property2 v2("test/uint32", std::uint32_t(12));
        v2 = v;
        EXPECT_EQ(v2.type(), Property2::Type::Double);
        EXPECT_FALSE(v2.empty());
        ASSERT_TRUE(!!v2.getDouble());
        EXPECT_DOUBLE_EQ(*v2.getDouble(), -0.1);
        EXPECT_EQ(v2.name(), std::string_view("test/double"));
    }

    // String
    {
        Property2 v("test/string", "from_char*");
        Property2 v1(v);
        EXPECT_EQ(v1.type(), Property2::Type::String);
        EXPECT_FALSE(v1.empty());
        ASSERT_TRUE(!!v1.getString());
        EXPECT_EQ(*v1.getString(), std::string_view("from_char*"));
        EXPECT_EQ(v1.name(), std::string_view("test/string"));

        Property2 v2("test/uint32", std::uint32_t(12));
        v2 = v;
        EXPECT_EQ(v2.type(), Property2::Type::String);
        EXPECT_FALSE(v2.empty());
        ASSERT_TRUE(!!v2.getString());
        EXPECT_EQ(*v2.getString(), std::string_view("from_char*"));
        EXPECT_EQ(v2.name(), std::string_view("test/string"));
    }

    // Binary
    {
        const Binary s("from_const Binary&");
        Property2 v("test/binary", s);
        Property2 v1(v);
        EXPECT_EQ(v1.type(), Property2::Type::Binary);
        EXPECT_FALSE(v1.empty());
        ASSERT_TRUE(!!v1.getBinary());
        EXPECT_EQ(*v1.getBinary(), s);
        EXPECT_EQ(v1.name(), std::string_view("test/binary"));

        Property2 v2("test/uint32", std::uint32_t(12));
        v2 = v;
        EXPECT_EQ(v2.type(), Property2::Type::Binary);
        EXPECT_FALSE(v2.empty());
        ASSERT_TRUE(!!v2.getBinary());
        EXPECT_EQ(*v2.getBinary(), s);
        EXPECT_EQ(v2.name(), std::string_view("test/binary"));
    }

    // Map
    {
        Property2 v;
        Property2 v1("test/bool", True);
        Property2 v2("test/int32", std::int32_t(-12));

        Property2::Map m;
        addProperty(m, v);
        addProperty(m, v1);
        addProperty(m, v2);
        
        Property2 p("test/map", m);
        Property2 p1(p);

        {
            EXPECT_EQ(p1.type(), Property2::Type::Map);
            EXPECT_FALSE(p1.empty());
            ASSERT_TRUE(!!p1.getMap());
            EXPECT_EQ(p1.name(), std::string_view("test/map"));
            auto& rm = *p1.getMap();
            ASSERT_EQ(rm.size(), 3);

            auto it = rm.find("");
            ASSERT_NE(it, rm.end());

            it = rm.find("test/bool");
            ASSERT_NE(it, rm.end());
            ASSERT_TRUE(!!it->second.getBool());
            EXPECT_EQ(*it->second.getBool(), True);

            it = rm.find("test/int32");
            ASSERT_NE(it, rm.end());
            ASSERT_TRUE(!!it->second.getInt32());
            EXPECT_EQ(*it->second.getInt32(), -12);
        }

        Property2 p2("test/uint32", std::uint32_t(12));
        p2 = p;

        {
            EXPECT_EQ(p2.type(), Property2::Type::Map);
            EXPECT_FALSE(p2.empty());
            ASSERT_TRUE(!!p2.getMap());
            EXPECT_EQ(p2.name(), std::string_view("test/map"));
            auto& rm = *p2.getMap();
            ASSERT_EQ(rm.size(), 3);

            auto it = rm.find("");
            ASSERT_NE(it, rm.end());

            it = rm.find("test/bool");
            ASSERT_NE(it, rm.end());
            ASSERT_TRUE(!!it->second.getBool());
            EXPECT_EQ(*it->second.getBool(), True);

            it = rm.find("test/int32");
            ASSERT_NE(it, rm.end());
            ASSERT_TRUE(!!it->second.getInt32());
            EXPECT_EQ(*it->second.getInt32(), -12);
        }
    }
}

TEST(Property2, Moving)
{
    {
        Property2 v;

        Property2 v1(std::move(v));
        EXPECT_TRUE(v.empty());
        EXPECT_FALSE(!!v.getBool());
        EXPECT_TRUE(v.name().empty());
        EXPECT_TRUE(v1.empty());
        EXPECT_FALSE(!!v1.getBool());
        EXPECT_TRUE(v1.name().empty());

        Property2 v2("test/uint32", std::uint32_t(12));
        v2 = std::move(v1);
        EXPECT_TRUE(v2.empty());
        EXPECT_FALSE(!!v2.getBool());
        EXPECT_TRUE(v2.name().empty());
        EXPECT_TRUE(v1.empty());
        EXPECT_FALSE(!!v1.getBool());
        EXPECT_TRUE(v1.name().empty());
    }

    {
        Property2 v("test/int32", std::int32_t(-12));

        Property2 v1(std::move(v));
        EXPECT_EQ(v1.type(), Property2::Type::Int32);
        EXPECT_FALSE(v1.empty());
        ASSERT_TRUE(!!v1.getInt32());
        EXPECT_EQ(*v1.getInt32(), -12);
        EXPECT_EQ(v1.name(), std::string_view("test/int32"));

        EXPECT_TRUE(v.empty());
        EXPECT_FALSE(!!v.getInt32());
        EXPECT_TRUE(v.name().empty());

        Property2 v2("test/uint32", std::uint32_t(12));
        v2 = std::move(v1);
        EXPECT_EQ(v2.type(), Property2::Type::Int32);
        EXPECT_FALSE(v2.empty());
        ASSERT_TRUE(!!v2.getInt32());
        EXPECT_EQ(*v2.getInt32(), -12);
        EXPECT_EQ(v2.name(), std::string_view("test/int32"));

        EXPECT_TRUE(v1.empty());
        EXPECT_FALSE(!!v1.getInt32());
        EXPECT_TRUE(v1.name().empty());
    }

    {
        Property2 v("test/string", "how are you?");

        Property2 v1(std::move(v));
        EXPECT_EQ(v1.type(), Property2::Type::String);
        EXPECT_FALSE(v1.empty());
        ASSERT_TRUE(!!v1.getString());
        EXPECT_EQ(*v1.getString(), std::string_view("how are you?"));
        EXPECT_EQ(v1.name(), std::string_view("test/string"));

        EXPECT_TRUE(v.empty());
        EXPECT_FALSE(!!v.getString());
        EXPECT_TRUE(v.name().empty());

        Property2 v2("test/uint32", std::uint32_t(12));
        v2 = std::move(v1);
        EXPECT_EQ(v2.type(), Property2::Type::String);
        EXPECT_FALSE(v2.empty());
        ASSERT_TRUE(!!v2.getString());
        EXPECT_EQ(*v2.getString(), std::string_view("how are you?"));
        EXPECT_EQ(v2.name(), std::string_view("test/string"));

        EXPECT_TRUE(v1.empty());
        EXPECT_FALSE(!!v1.getString());
        EXPECT_TRUE(v1.name().empty());
    }

    {
        Property2 v;
        Property2 v1("test/bool", True);
        Property2 v2("test/int32", std::int32_t(-12));

        Property2::Map m;
        addProperty(m, v);
        addProperty(m, v1);
        addProperty(m, v2);

        Property2 p("test/map", m);
        Property2 p1(std::move(p));

        {
            EXPECT_EQ(p1.type(), Property2::Type::Map);
            EXPECT_FALSE(p1.empty());
            ASSERT_TRUE(!!p1.getMap());
            EXPECT_EQ(p1.name(), std::string_view("test/map"));
            auto& rm = *p1.getMap();
            ASSERT_EQ(rm.size(), 3);

            auto it = rm.find("");
            ASSERT_NE(it, rm.end());

            it = rm.find("test/bool");
            ASSERT_NE(it, rm.end());
            ASSERT_TRUE(!!it->second.getBool());
            EXPECT_EQ(*it->second.getBool(), True);

            it = rm.find("test/int32");
            ASSERT_NE(it, rm.end());
            ASSERT_TRUE(!!it->second.getInt32());
            EXPECT_EQ(*it->second.getInt32(), -12);
        }

        EXPECT_TRUE(p.empty());
        EXPECT_FALSE(!!p.getMap());
        EXPECT_TRUE(p.name().empty());

        Property2 p2("test/uint32", std::uint32_t(12));
        p2 = std::move(p1);

        {
            EXPECT_EQ(p2.type(), Property2::Type::Map);
            EXPECT_FALSE(p2.empty());
            ASSERT_TRUE(!!p2.getMap());
            EXPECT_EQ(p2.name(), std::string_view("test/map"));
            auto& rm = *p2.getMap();
            ASSERT_EQ(rm.size(), 3);

            auto it = rm.find("");
            ASSERT_NE(it, rm.end());

            it = rm.find("test/bool");
            ASSERT_NE(it, rm.end());
            ASSERT_TRUE(!!it->second.getBool());
            EXPECT_EQ(*it->second.getBool(), True);

            it = rm.find("test/int32");
            ASSERT_NE(it, rm.end());
            ASSERT_TRUE(!!it->second.getInt32());
            EXPECT_EQ(*it->second.getInt32(), -12);
        }

        EXPECT_TRUE(p1.empty());
        EXPECT_FALSE(!!p1.getMap());
        EXPECT_TRUE(p1.name().empty());
    }
}

TEST(Property2, Equality)
{
    // Empty
    Property2 vEmpty1;
    Property2 vEmpty2;
    EXPECT_TRUE(vEmpty1 == vEmpty1);
    EXPECT_TRUE(vEmpty1 == vEmpty2);

    Property2 vBool1("test/bool", True);
    Property2 vBool2("test/bool", True);
    Property2 vBool3("test/bool", False);
    EXPECT_FALSE(vBool1 == vEmpty1);
    EXPECT_TRUE(vBool1 == vBool1);
    EXPECT_TRUE(vBool1 == vBool2);
    EXPECT_FALSE(vBool1 == vBool3);
        
    Property2 vInt321("test/int32", std::int32_t(-12));
    Property2 vInt322("test/int32", std::int32_t(-12));
    Property2 vInt323("test/int32", std::int32_t(1));
    EXPECT_FALSE(vInt321 == vBool1);
    EXPECT_TRUE(vInt321 == vInt321);
    EXPECT_TRUE(vInt321 == vInt322);
    EXPECT_FALSE(vInt321 == vInt323);

    Property2 vUInt321("test/uint32", std::uint32_t(0xf0000001));
    Property2 vUInt322("test/uint32", std::uint32_t(0xf0000001));
    Property2 vUInt323("test/uint32", std::uint32_t(0x80000001));
    EXPECT_FALSE(vUInt321 == vInt321);
    EXPECT_TRUE(vUInt321 == vUInt321);
    EXPECT_TRUE(vUInt321 == vUInt322);
    EXPECT_FALSE(vUInt321 == vInt323);

    Property2 vInt641("test/int64", std::int64_t(-9223372036854775803LL));
    Property2 vInt642("test/int64", std::int64_t(-9223372036854775803LL));
    Property2 vInt643("test/int64", std::int64_t(11LL));
    EXPECT_FALSE(vInt641 == vUInt321);
    EXPECT_TRUE(vInt641 == vInt641);
    EXPECT_TRUE(vInt641 == vInt642);
    EXPECT_FALSE(vInt641 == vInt643);
        
    Property2 vUInt641("test/uint64", std::uint64_t(0x8000000000000005ULL));
    Property2 vUInt642("test/uint64", std::uint64_t(0x8000000000000005ULL));
    Property2 vUInt643("test/uint64", std::uint64_t(1ULL));
    EXPECT_FALSE(vUInt641 == vInt641);
    EXPECT_TRUE(vUInt641 == vUInt641);
    EXPECT_TRUE(vUInt641 == vUInt642);
    EXPECT_FALSE(vUInt641 == vUInt643);

    Property2 vDouble1("test/double", -0.1);
    Property2 vDouble2("test/double", -0.1);
    Property2 vDouble3("test/double", 1.0);
    EXPECT_FALSE(vDouble1 == vUInt641);
    EXPECT_TRUE(vDouble1 == vDouble1);
    EXPECT_TRUE(vDouble1 == vDouble2);
    EXPECT_FALSE(vDouble1 == vDouble3);
    
    Property2 vString1("test/string", "xaxaxa");
    Property2 vString2("test/string", "xaxaxa");
    Property2 vString3("test/string", "xyxyxy");
    EXPECT_FALSE(vString1 == vDouble1);
    EXPECT_TRUE(vString1 == vString1);
    EXPECT_TRUE(vString1 == vString2);
    EXPECT_FALSE(vString1 == vString3);
    
    const Binary s1("xaxaxa");
    const Binary s2("uxuxu");
    Property2 vBinary1("test/binary", s1);
    Property2 vBinary2("test/binary", s1);
    Property2 vBinary3("test/binary", s2);
    EXPECT_FALSE(vBinary1 == vString1);
    EXPECT_TRUE(vBinary1 == vBinary1);
    EXPECT_TRUE(vBinary1 == vBinary2);
    EXPECT_FALSE(vBinary1 == vBinary3);
        

    Property2::Map m1;
    Property2::Map m2;
    {
        {
            Property2 v;
            Property2 v1("test/bool", True);
            Property2 v2("test/int32", std::int32_t(-12));
            addProperty(m1, v);
            addProperty(m1, v1);
            addProperty(m1, v2);

        }
        {
            Property2 v;
            Property2 v1("test/bool", False);
            Property2 v2("test/int32", std::int32_t(-12));

            addProperty(m2, v);
            addProperty(m2, v1);
            addProperty(m2, v2);

        }
    }

    Property2 vMap1("test/map", m1);
    Property2 vMap2("test/map", m1);
    Property2 vMap3("test/map", m2);
    EXPECT_FALSE(vMap1 == vString1);
    EXPECT_TRUE(vMap1 == vMap1);
    EXPECT_TRUE(vMap1 == vMap2);
    EXPECT_FALSE(vMap1 == vMap3);
}

TEST(Property2, str)
{
    Property2 top;
    {
        Property2::Map m0;
        m0.insert({ "top/empty", Property2() });
        m0.insert({ "top/int32", Property2("top/int32", std::int32_t(-12)) });
        m0.insert({ "top/uint32", Property2("top/uint32", std::int32_t(121)) });

        {
            Property2::Map m1;
            m1.insert({ "level1/empty", Property2() });
            m1.insert({ "level1/double", Property2("level1/double", -0.2) });
            m1.insert({ "level1/string", Property2("level1/string", std::string("xa xa xa")) });

            m0.insert({ "top/map", Property2("top/map", std::move(m1)) });
        }

        m0.insert({ "top/binary", Property2("top/binary", Binary(std::string("ox ox ox"))) });
        
        top = Property2("top", std::move(m0));
    }

    auto s = top.str();
    EXPECT_STREQ(
        s.c_str(),
R"([ { "top/binary" = "6F 78 20 6F 78 20 6F 78" }, { "top/empty" = "[empty]" }, { "top/int32" = "-12" }, { "top/map" = "[ { "level1/double" = "-0.200000" }, { "level1/empty" = "[empty]" }, { "level1/string" = "xa xa xa" } ]" }, { "top/uint32" = "121" } ])"
    );
}

TEST(Property2, visit)
{
    PropertyMap m;
    addProperty(m, {});
    addProperty(m, { "test/bool", True });
    addProperty(m, { "test/int32", std::int32_t(-12) });
    addProperty(m, { "test/uint32", std::uint32_t(13) });
    addProperty(m, { "test/int64", std::int64_t(-125) });
    addProperty(m, { "test/uint64", std::uint64_t(555) });
    addProperty(m, { "test/double", -0.1 });
    addProperty(m, { "test/string", std::string("xa xa xa") });
    addProperty(m, { "test/binary", Binary(std::string("xo xo xo")) });

    PropertyMap m1;
    addProperty(m1, { "test/int32", std::int32_t(-99) });
    addProperty(m1, { "test/string", std::string("uxa uxa uxa") });
    addProperty(m, { "test/map", m1 });

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

        PropertyMap m;
        PropertyMap m1;

        bool operator()(const Property2::Name& name, const Empty& v)
        {
            empty_visited = name.empty();
            addProperty(m, {});
            return true;
        }

        bool operator()(const Property2::Name& name, const Bool& v)
        {
            bool_visited = (name == Property2::Name("test/bool")) && (v == True);
            addProperty(m, { name, v });
            return true;
        }

        bool operator()(const Property2::Name& name, const std::int32_t& v)
        {
            int32_visited = (name == Property2::Name("test/int32")) && (v == -12);
            addProperty(m, { name, v });
            return true;
        }

        bool operator()(const Property2::Name& name, const std::uint32_t& v)
        {
            uint32_visited = (name == Property2::Name("test/uint32")) && (v == 13);
            addProperty(m, { name, v });
            return true;
        }

        bool operator()(const Property2::Name& name, const std::int64_t& v)
        {
            int64_visited = (name == Property2::Name("test/int64")) && (v == -125);
            addProperty(m, { name, v });
            return true;
        }

        bool operator()(const Property2::Name& name, const std::uint64_t& v)
        {
            uint64_visited = (name == Property2::Name("test/uint64")) && (v == 555);
            addProperty(m, { name, v });
            return true;
        }

        bool operator()(const Property2::Name& name, const double& v)
        {
            double_visited = (name == Property2::Name("test/double")) && (v == -0.1);
            addProperty(m, { name, v });
            return true;
        }

        bool operator()(const Property2::Name& name, const std::string& v)
        {
            string_visited = (name == Property2::Name("test/string")) && (v == std::string("xa xa xa"));
            addProperty(m, { name, v });
            return true;
        }

        bool operator()(const Property2::Name& name, const Binary& v)
        {
            binary_visited = (name == Property2::Name("test/binary")) && (v == Binary(std::string("xo xo xo")));
            addProperty(m, { name, v });
            return true;
        }

        bool operator()(const Property2::Name& name, const PropertyMap& v)
        {
            map_visited = (name == Property2::Name("test/map"));

            for (auto& p : v)
            {
                addProperty(m1, p.second);
            }

            addProperty(m, { name, v });
            return true;
        }
    };

    Visitor vis;
    visit(m, vis);

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

    EXPECT_TRUE(vis.m == m);
    EXPECT_TRUE(vis.m1 == m1);
}

TEST(Property2, find)
{
    PropertyMap m;
    addProperty(m, {});
    addProperty(m, { "test/bool", True });
    addProperty(m, { "test/int32", std::int32_t(-12) });
    addProperty(m, { "test/uint32", std::uint32_t(13) });
    addProperty(m, { "test/int64", std::int64_t(-125) });
    addProperty(m, { "test/uint64", std::uint64_t(555) });
    addProperty(m, { "test/double", -0.1 });
    addProperty(m, { "test/string", std::string("xa xa xa") });
    addProperty(m, { "test/binary", Binary(std::string("xo xo xo")) });

    PropertyMap m1;
    addProperty(m1, { "test/int32", std::int32_t(-99) });
    addProperty(m1, { "test/string", std::string("uxa uxa uxa") });
    addProperty(m, { "test/map", m1 });

    auto p = findProperty(m, "");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property2::Type::Empty);

    p = findProperty(m, "test/bool");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property2::Type::Bool);
    EXPECT_EQ(*p->getBool(), True);

    p = findProperty(m, "test/int32");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property2::Type::Int32);
    EXPECT_EQ(*p->getInt32(), -12);

    p = findProperty(m, "test/uint32");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property2::Type::UInt32);
    EXPECT_EQ(*p->getUInt32(), 13);

    p = findProperty(m, "test/int64");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property2::Type::Int64);
    EXPECT_EQ(*p->getInt64(), -125);

    p = findProperty(m, "test/uint64");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property2::Type::UInt64);
    EXPECT_EQ(*p->getUInt64(), 555);

    p = findProperty(m, "test/double");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property2::Type::Double);
    EXPECT_DOUBLE_EQ(*p->getDouble(), -0.1);

    p = findProperty(m, "test/string");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property2::Type::String);
    EXPECT_EQ(*p->getString(), std::string("xa xa xa"));

    p = findProperty(m, "test/binary");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property2::Type::Binary);
    EXPECT_EQ(*p->getBinary(), Binary(std::string("xo xo xo")));

    p = findProperty(m, "test/map");
    ASSERT_TRUE(!!p);
    EXPECT_TRUE(p->type() == Property2::Type::Map);
    EXPECT_TRUE(*p->getMap() == m1);
}