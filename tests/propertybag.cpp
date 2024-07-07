#include "common.hpp"

#include <erebus/propertybag.hxx>


TEST(Er_PropertyBag, simple)
{
    Er::PropertyBag bag;

    {
        std::string str("some string");
        Er::addProperty<TestProps::StringProp>(bag, str);
        EXPECT_FALSE(str.empty());
    }

    Er::addProperty<TestProps::Int32Prop>(bag, -5);

    ASSERT_TRUE(Er::propertyPresent<TestProps::StringProp>(bag));
    ASSERT_TRUE(Er::propertyPresent<TestProps::Int32Prop>(bag));
    EXPECT_FALSE(Er::propertyPresent<TestProps::DoubleProp>(bag));

    {
        auto sp = Er::getProperty<TestProps::StringProp>(bag);
        ASSERT_TRUE(sp);
        EXPECT_STREQ(sp->c_str(), "some string");
    }

    {
        auto ip = Er::getProperty<TestProps::Int32Prop>(bag);
        ASSERT_TRUE(ip);
        EXPECT_EQ(*ip, -5);
    }

    // cannnot modify prop
    {
        std::string str("another string");
        Er::addProperty<TestProps::StringProp>(bag, std::move(str));
        EXPECT_TRUE(str.empty()); // should have moved

        ASSERT_TRUE(Er::propertyPresent<TestProps::StringProp>(bag));
        auto sp = Er::getProperty<TestProps::StringProp>(bag);
        ASSERT_TRUE(sp);
        EXPECT_STREQ(sp->c_str(), "some string"); // should keep the old val
    }

    EXPECT_EQ(Er::getPropertyOr<TestProps::Int32Prop>(bag, 10), -5);
    EXPECT_EQ(Er::getPropertyOr<TestProps::DoubleProp>(bag, 10.0), 10.0);

    {
        std::string sd("default string");
        EXPECT_STREQ(Er::getPropertyOr<TestProps::StringProp>(bag, sd).c_str(), "some string");
        EXPECT_FALSE(sd.empty());

        Er::PropertyBag empty;
        EXPECT_STREQ(Er::getPropertyOr<TestProps::StringProp>(empty, std::move(sd)).c_str(), "default string");
        EXPECT_TRUE(sd.empty());
    }
}