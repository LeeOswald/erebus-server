#include "common.hpp"

#include <erebus/tree.hxx>

#include <vector>


namespace
{


template <typename KeyT>
struct Item
{
    using Key = KeyT;

    Key id;
    Key parentId;
    std::string name;
    void* _context = nullptr;

    template <typename _KeyT, typename _NameT>
    Item(_KeyT&& id, _KeyT&& parentId, _NameT&& name)
        : id(std::forward<_KeyT&&>(id))
        , parentId(std::forward<_KeyT&&>(parentId))
        , name(std::forward<_NameT&&>(name))
    {}

    const KeyT& key() const noexcept
    {
        return id;
    }

    const KeyT& parentKey() const noexcept
    {
        return parentId;
    }

    KeyT& parentKey() noexcept
    {
        return parentId;
    }

    bool isRoot() const noexcept
    {
        return (id == parentId);
    }

    void* const& context() const noexcept
    {
        return _context;
    }

    void*& context() noexcept
    {
        return _context;
    }
};


/*
 * Sample tree used in tests below
 *
 * -[1]---[2]
 *      |
 *      |-[3]-[4]
 *      |
 *      |-[5]---[6]
 *      |     |-[7]
 *      |     |-[8]
 *      |     |-[9]
 *      |
 *      -[10]
 *
 * -[11]---[12]
 * -[13]
 */


using ItemIntkey = Item<int>;

std::vector<std::shared_ptr<ItemIntkey>> createItemVector()
{
    std::vector<std::shared_ptr<ItemIntkey>> v;

    v.emplace_back(std::make_shared<ItemIntkey>(1, 1, "1"));
    v.emplace_back(std::make_shared<ItemIntkey>(2, 1 ,"2"));
    v.emplace_back(std::make_shared<ItemIntkey>(3, 1, "3"));
    v.emplace_back(std::make_shared<ItemIntkey>(4, 3, "4"));
    v.emplace_back(std::make_shared<ItemIntkey>(5, 1, "5"));
    v.emplace_back(std::make_shared<ItemIntkey>(6, 5, "6"));
    v.emplace_back(std::make_shared<ItemIntkey>(7, 5, "7"));
    v.emplace_back(std::make_shared<ItemIntkey>(8, 5, "8"));
    v.emplace_back(std::make_shared<ItemIntkey>(9, 5, "9"));
    v.emplace_back(std::make_shared<ItemIntkey>(10, 1, "10"));
    v.emplace_back(std::make_shared<ItemIntkey>(11, 11, "11"));
    v.emplace_back(std::make_shared<ItemIntkey>(12, 11, "12"));
    v.emplace_back(std::make_shared<ItemIntkey>(13, 13, "13"));

    return v;
}


using ItemStringKey = Item<std::string>;

std::map<std::string, std::shared_ptr<ItemStringKey>> createItemMap()
{
    std::map<std::string, std::shared_ptr<ItemStringKey>> m;

    m.emplace("1", std::make_shared<ItemStringKey>(std::string("1"), std::string("1"), "1"));
    m.emplace("2", std::make_shared<ItemStringKey>(std::string("2"), std::string("1"), "2"));
    m.emplace("3", std::make_shared<ItemStringKey>(std::string("3"), std::string("1"), "3"));
    m.emplace("4", std::make_shared<ItemStringKey>(std::string("4"), std::string("3"), "4"));
    m.emplace("5", std::make_shared<ItemStringKey>(std::string("5"), std::string("1"), "5"));
    m.emplace("6", std::make_shared<ItemStringKey>(std::string("6"), std::string("5"), "6"));
    m.emplace("7", std::make_shared<ItemStringKey>(std::string("7"), std::string("5"), "7"));
    m.emplace("8", std::make_shared<ItemStringKey>(std::string("8"), std::string("5"), "8"));
    m.emplace("9", std::make_shared<ItemStringKey>(std::string("9"), std::string("5"), "9"));
    m.emplace("10", std::make_shared<ItemStringKey>(std::string("10"), std::string("1"), "10"));
    m.emplace("11", std::make_shared<ItemStringKey>(std::string("11"), std::string("11"), "11"));
    m.emplace("12", std::make_shared<ItemStringKey>(std::string("12"), std::string("11"), "12"));
    m.emplace("13", std::make_shared<ItemStringKey>(std::string("13"), std::string("13"), "13"));

    return m;
}

} // namespace {}

TEST(Tree, insert)
{
    Er::Tree<ItemIntkey> t;

    EXPECT_EQ(t.size(), 0);
    EXPECT_TRUE(t.empty());

    auto _1 = std::make_shared<ItemIntkey>(1, 1, "1");
    auto _2 = std::make_shared<ItemIntkey>(2, 1 ,"2");
    auto _3 = std::make_shared<ItemIntkey>(3, 1, "3");
    auto _4 = std::make_shared<ItemIntkey>(4, 3, "4");
    auto _5 = std::make_shared<ItemIntkey>(5, 1, "5");
    auto _6 = std::make_shared<ItemIntkey>(6, 5, "6");
    auto _7 = std::make_shared<ItemIntkey>(7, 5, "7");
    auto _8 = std::make_shared<ItemIntkey>(8, 5, "8");
    auto _9 = std::make_shared<ItemIntkey>(9, 5, "9");
    auto _10 = std::make_shared<ItemIntkey>(10, 1, "10");
    auto _11 = std::make_shared<ItemIntkey>(11, 11, "11");
    auto _12 = std::make_shared<ItemIntkey>(12, 11, "12");
    auto _13 = std::make_shared<ItemIntkey>(13, 13, "13");

    auto n1 = t.insert(_1);
    EXPECT_EQ(n1->parent(), t.root());
    EXPECT_EQ(t.size(), 1);
    EXPECT_TRUE(!t.empty());

    auto n2 = t.insert(_2);
    EXPECT_EQ(n2->parent()->data(), _1.get());
    EXPECT_EQ(t.size(), 2);
    EXPECT_TRUE(!t.empty());

    auto n4 = t.insert(_4);
    EXPECT_EQ(n4->parent(), t.root());
    EXPECT_EQ(t.size(), 3);
    EXPECT_TRUE(!t.empty());

    auto n3 = t.insert(_3);
    EXPECT_EQ(n3->parent()->data(), _1.get());
    EXPECT_EQ(t.size(), 4);
    EXPECT_TRUE(!t.empty());

    auto n6 = t.insert(_6);
    EXPECT_EQ(n6->parent(), t.root());
    EXPECT_EQ(t.size(), 5);
    EXPECT_TRUE(!t.empty());

    auto n7 = t.insert(_7);
    EXPECT_EQ(n7->parent(), t.root());
    EXPECT_EQ(t.size(), 6);
    EXPECT_TRUE(!t.empty());

    auto n5 = t.insert(_5);
    EXPECT_EQ(n5->parent()->data(), _1.get());
    EXPECT_EQ(t.size(), 7);
    EXPECT_TRUE(!t.empty());
    EXPECT_EQ(n6->parent()->data(), _5.get());
    EXPECT_EQ(n7->parent()->data(), _5.get());

    auto n8 = t.insert(_8);
    EXPECT_EQ(n8->parent()->data(), _5.get());
    EXPECT_EQ(t.size(), 8);
    EXPECT_TRUE(!t.empty());

    auto n9 = t.insert(_9);
    EXPECT_EQ(n8->parent()->data(), _5.get());
    EXPECT_EQ(t.size(), 9);
    EXPECT_TRUE(!t.empty());

    auto n10 = t.insert(_10);
    EXPECT_EQ(n10->parent()->data(), _1.get());
    EXPECT_EQ(t.size(), 10);
    EXPECT_TRUE(!t.empty());

    auto n11 = t.insert(_11);
    EXPECT_EQ(n11->parent(), t.root());
    EXPECT_EQ(t.size(), 11);
    EXPECT_TRUE(!t.empty());

    auto n13 = t.insert(_13);
    EXPECT_EQ(n13->parent(), t.root());
    EXPECT_EQ(t.size(), 12);
    EXPECT_TRUE(!t.empty());

    auto n12 = t.insert(_12);
    EXPECT_EQ(n12->parent()->data(), _11.get());
    EXPECT_EQ(t.size(), 13);
    EXPECT_TRUE(!t.empty());

    EXPECT_EQ(t.find(1)->data(), _1.get());
    EXPECT_EQ(t.find(2)->data(), _2.get());
    EXPECT_EQ(t.find(3)->data(), _3.get());
    EXPECT_EQ(t.find(4)->data(), _4.get());
    EXPECT_EQ(t.find(5)->data(), _5.get());
    EXPECT_EQ(t.find(6)->data(), _6.get());
    EXPECT_EQ(t.find(7)->data(), _7.get());
    EXPECT_EQ(t.find(8)->data(), _8.get());
    EXPECT_EQ(t.find(9)->data(), _9.get());
    EXPECT_EQ(t.find(10)->data(), _10.get());
    EXPECT_EQ(t.find(11)->data(), _11.get());
    EXPECT_EQ(t.find(12)->data(), _12.get());
    EXPECT_EQ(t.find(13)->data(), _13.get());

    EXPECT_EQ(n1->children().size(), 4);
    EXPECT_EQ(n2->children().size(), 0);
    EXPECT_EQ(n3->children().size(), 1);
    EXPECT_EQ(n4->children().size(), 0);
    EXPECT_EQ(n5->children().size(), 4);
    EXPECT_EQ(n6->children().size(), 0);
    EXPECT_EQ(n7->children().size(), 0);
    EXPECT_EQ(n8->children().size(), 0);
    EXPECT_EQ(n9->children().size(), 0);
    EXPECT_EQ(n10->children().size(), 0);
    EXPECT_EQ(n11->children().size(), 1);
    EXPECT_EQ(n12->children().size(), 0);
    EXPECT_EQ(n13->children().size(), 0);
}

TEST(Tree, remove)
{
    Er::Tree<ItemIntkey> t(createItemVector());

    auto n1 = t.find(1);
    EXPECT_TRUE(n1);
    t.remove(n1->data());
    EXPECT_EQ(t.size(), 12);
    EXPECT_FALSE(t.find(1));

    auto n2 = t.find(2);
    EXPECT_TRUE(n2);
    EXPECT_EQ(n2->parent(), t.root());

    auto n3 = t.find(3);
    EXPECT_TRUE(n3);
    EXPECT_EQ(n3->parent(), t.root());

    auto n5 = t.find(5);
    EXPECT_TRUE(n5);
    EXPECT_EQ(n5->parent(), t.root());

    auto n10 = t.find(10);
    EXPECT_TRUE(n10);
    EXPECT_EQ(n10->parent(), t.root());

    t.remove(n2->data());
    EXPECT_EQ(t.size(), 11);
    EXPECT_FALSE(t.find(2));

    auto n4 = t.find(4);
    EXPECT_TRUE(n4);
    EXPECT_EQ(n4->parent(), n3);

    t.remove(n4->data());
    EXPECT_EQ(t.size(), 10);
    EXPECT_FALSE(t.find(4));

    t.remove(n3->data());
    EXPECT_EQ(t.size(), 9);
    EXPECT_FALSE(t.find(3));

    t.remove(n5->data());
    EXPECT_EQ(t.size(), 8);
    EXPECT_FALSE(t.find(5));

    auto n6 = t.find(6);
    EXPECT_TRUE(n6);
    EXPECT_EQ(n6->parent(), t.root());

    auto n7 = t.find(7);
    EXPECT_TRUE(n7);
    EXPECT_EQ(n7->parent(), t.root());

    auto n8 = t.find(8);
    EXPECT_TRUE(n8);
    EXPECT_EQ(n8->parent(), t.root());

    auto n9 = t.find(9);
    EXPECT_TRUE(n9);
    EXPECT_EQ(n9->parent(), t.root());

    t.remove(n6->data());
    EXPECT_EQ(t.size(), 7);
    EXPECT_FALSE(t.find(6));

    t.remove(n7->data());
    EXPECT_EQ(t.size(), 6);
    EXPECT_FALSE(t.find(7));

    t.remove(n8->data());
    EXPECT_EQ(t.size(), 5);
    EXPECT_FALSE(t.find(8));

    t.remove(n9->data());
    EXPECT_EQ(t.size(), 4);
    EXPECT_FALSE(t.find(9));

    t.remove(n10->data());
    EXPECT_EQ(t.size(), 3);
    EXPECT_FALSE(t.find(10));

    auto n11 = t.find(11);
    EXPECT_TRUE(n11);
    EXPECT_EQ(n11->parent(), t.root());

    auto n12 = t.find(12);
    EXPECT_TRUE(n12);
    EXPECT_EQ(n12->parent(), n11);

    auto n13 = t.find(13);
    EXPECT_TRUE(n13);
    EXPECT_EQ(n13->parent(), t.root());

    t.remove(n12->data());
    EXPECT_EQ(t.size(), 2);
    EXPECT_FALSE(t.find(12));

    t.remove(n11->data());
    EXPECT_EQ(t.size(), 1);
    EXPECT_FALSE(t.find(11));

    t.remove(n13->data());
    EXPECT_EQ(t.size(), 0);
    EXPECT_FALSE(t.find(13));
    EXPECT_TRUE(t.empty());
}

TEST(Tree, intKeysCreateFromVector)
{
    auto v = createItemVector();

    EXPECT_EQ(v.size(), 13);

    Er::Tree<ItemIntkey> t(v);
    using NodeT = Er::Tree<ItemIntkey>::Node;

    EXPECT_EQ(t.size(), 13);

    auto root = t.root();
    EXPECT_EQ(root->parent(), nullptr);
    EXPECT_EQ(root->data(), nullptr);

    auto& roots = root->children();
    EXPECT_EQ(roots.size(), 3);

    NodeT* _1 = nullptr;
    NodeT* _2 = nullptr;
    NodeT* _3 = nullptr;
    NodeT* _4 = nullptr;
    NodeT* _5 = nullptr;
    NodeT* _6 = nullptr;
    NodeT* _7 = nullptr;
    NodeT* _8 = nullptr;
    NodeT* _9 = nullptr;
    NodeT* _10 = nullptr;
    NodeT* _11 = nullptr;
    NodeT* _12 = nullptr;
    NodeT* _13 = nullptr;

    for (auto& i: roots)
    {
        if (i->data()->id == 1)
        {
            EXPECT_EQ(_1, nullptr);
            _1 = i;
        }
        else if (i->data()->id == 11)
        {
            EXPECT_EQ(_11, nullptr);
            _11 = i;
        }
        else if (i->data()->id == 13)
        {
            EXPECT_EQ(_13, nullptr);
            _13 = i;
        }
        else
        {
            EXPECT_EQ(true, false);
        }
    }

    EXPECT_NE(_1, nullptr);
    EXPECT_NE(_11, nullptr);
    EXPECT_NE(_13, nullptr);

    EXPECT_EQ(root->child(root->indexOfChild(_1)), _1);
    EXPECT_EQ(root->child(root->indexOfChild(_11)), _11);
    EXPECT_EQ(root->child(root->indexOfChild(_13)), _13);

    // non-existent child
    EXPECT_EQ(root->indexOfChild(root), NodeT::InvalidIndex);
    EXPECT_EQ(root->child(NodeT::InvalidIndex), nullptr);

    EXPECT_EQ(_1->parent(), root);
    EXPECT_NE(_1->data(), nullptr);
    EXPECT_EQ(_1->data()->id, 1);
    EXPECT_EQ(_1->data()->parentId, 1);
    EXPECT_STREQ(_1->data()->name.c_str(), "1");

    EXPECT_EQ(_11->parent(), root);
    EXPECT_NE(_11->data(), nullptr);
    EXPECT_EQ(_11->data()->id, 11);
    EXPECT_EQ(_11->data()->parentId, 11);
    EXPECT_STREQ(_11->data()->name.c_str(), "11");

    EXPECT_EQ(_13->parent(), root);
    EXPECT_NE(_13->data(), nullptr);
    EXPECT_EQ(_13->data()->id, 13);
    EXPECT_EQ(_13->data()->parentId, 13);
    EXPECT_STREQ(_13->data()->name.c_str(), "13");

    auto& _1_children = _1->children();
    EXPECT_EQ(_1_children.size(), 4);

    for (auto& i: _1_children)
    {
        if (i->data()->id == 2)
        {
            EXPECT_EQ(_2, nullptr);
            _2 = i;
        }
        else if (i->data()->id == 3)
        {
            EXPECT_EQ(_3, nullptr);
            _3 = i;
        }
        else if (i->data()->id == 5)
        {
            EXPECT_EQ(_5, nullptr);
            _5 = i;
        }
        else if (i->data()->id == 10)
        {
            EXPECT_EQ(_10, nullptr);
            _10 = i;
        }
        else
        {
            EXPECT_EQ(true, false);
        }
    }

    EXPECT_NE(_2, nullptr);
    EXPECT_NE(_3, nullptr);
    EXPECT_NE(_5, nullptr);
    EXPECT_NE(_10, nullptr);

    EXPECT_EQ(_1->child(_1->indexOfChild(_2)), _2);
    EXPECT_EQ(_1->child(_1->indexOfChild(_3)), _3);
    EXPECT_EQ(_1->child(_1->indexOfChild(_5)), _5);
    EXPECT_EQ(_1->child(_1->indexOfChild(_10)), _10);

    EXPECT_EQ(_2->parent(), _1);
    EXPECT_NE(_2->data(), nullptr);
    EXPECT_EQ(_2->data()->id, 2);
    EXPECT_EQ(_2->data()->parentId, 1);
    EXPECT_STREQ(_2->data()->name.c_str(), "2");

    EXPECT_EQ(_3->parent(), _1);
    EXPECT_NE(_3->data(), nullptr);
    EXPECT_EQ(_3->data()->id, 3);
    EXPECT_EQ(_3->data()->parentId, 1);
    EXPECT_STREQ(_3->data()->name.c_str(), "3");

    EXPECT_EQ(_5->parent(), _1);
    EXPECT_NE(_5->data(), nullptr);
    EXPECT_EQ(_5->data()->id, 5);
    EXPECT_EQ(_5->data()->parentId, 1);
    EXPECT_STREQ(_5->data()->name.c_str(), "5");

    EXPECT_EQ(_10->parent(), _1);
    EXPECT_NE(_10->data(), nullptr);
    EXPECT_EQ(_10->data()->id, 10);
    EXPECT_EQ(_10->data()->parentId, 1);
    EXPECT_STREQ(_10->data()->name.c_str(), "10");

    EXPECT_TRUE(_2->children().empty());

    auto& _3_children = _3->children();
    EXPECT_EQ(_3_children.size(), 1);

    for (auto& i: _3_children)
    {
        if (i->data()->id == 4)
        {
            EXPECT_EQ(_4, nullptr);
            _4 = i;
        }
        else
        {
            EXPECT_EQ(true, false);
        }
    }

    EXPECT_NE(_4, nullptr);
    EXPECT_EQ(_3->child(_3->indexOfChild(_4)), _4);
    EXPECT_EQ(_4->parent(), _3);
    EXPECT_NE(_4->data(), nullptr);
    EXPECT_EQ(_4->data()->id, 4);
    EXPECT_EQ(_4->data()->parentId, 3);
    EXPECT_STREQ(_4->data()->name.c_str(), "4");

    auto& _5_children = _5->children();
    EXPECT_EQ(_5_children.size(), 4);

    for (auto& i: _5_children)
    {
        if (i->data()->id == 6)
        {
            EXPECT_EQ(_6, nullptr);
            _6 = i;
        }
        else if (i->data()->id == 7)
        {
            EXPECT_EQ(_7, nullptr);
            _7 = i;
        }
        else if (i->data()->id == 8)
        {
            EXPECT_EQ(_8, nullptr);
            _8 = i;
        }
        else if (i->data()->id == 9)
        {
            EXPECT_EQ(_9, nullptr);
            _9 = i;
        }
        else
        {
            EXPECT_EQ(true, false);
        }
    }

    EXPECT_NE(_6, nullptr);
    EXPECT_EQ(_5->child(_5->indexOfChild(_6)), _6);
    EXPECT_EQ(_6->parent(), _5);
    EXPECT_NE(_6->data(), nullptr);
    EXPECT_EQ(_6->data()->id, 6);
    EXPECT_EQ(_6->data()->parentId, 5);
    EXPECT_STREQ(_6->data()->name.c_str(), "6");

    EXPECT_NE(_7, nullptr);
    EXPECT_EQ(_5->child(_5->indexOfChild(_7)), _7);
    EXPECT_EQ(_7->parent(), _5);
    EXPECT_NE(_7->data(), nullptr);
    EXPECT_EQ(_7->data()->id, 7);
    EXPECT_EQ(_7->data()->parentId, 5);
    EXPECT_STREQ(_7->data()->name.c_str(), "7");

    EXPECT_NE(_8, nullptr);
    EXPECT_EQ(_5->child(_5->indexOfChild(_8)), _8);
    EXPECT_EQ(_8->parent(), _5);
    EXPECT_NE(_8->data(), nullptr);
    EXPECT_EQ(_8->data()->id, 8);
    EXPECT_EQ(_8->data()->parentId, 5);
    EXPECT_STREQ(_8->data()->name.c_str(), "8");

    EXPECT_NE(_9, nullptr);
    EXPECT_EQ(_5->child(_5->indexOfChild(_9)), _9);
    EXPECT_EQ(_9->parent(), _5);
    EXPECT_NE(_9->data(), nullptr);
    EXPECT_EQ(_9->data()->id, 9);
    EXPECT_EQ(_9->data()->parentId, 5);
    EXPECT_STREQ(_9->data()->name.c_str(), "9");

    EXPECT_TRUE(_10->children().empty());

    auto& _11_children = _11->children();
    EXPECT_EQ(_11_children.size(), 1);

    for (auto& i: _11_children)
    {
        if (i->data()->id == 12)
        {
            EXPECT_EQ(_12, nullptr);
            _12 = i;
        }
        else
        {
            EXPECT_EQ(true, false);
        }
    }

    EXPECT_NE(_12, nullptr);
    EXPECT_EQ(_11->child(_11->indexOfChild(_12)), _12);
    EXPECT_EQ(_12->parent(), _11);
    EXPECT_NE(_12->data(), nullptr);
    EXPECT_EQ(_12->data()->id, 12);
    EXPECT_EQ(_12->data()->parentId, 11);
    EXPECT_STREQ(_12->data()->name.c_str(), "12");

    EXPECT_TRUE(_13->children().empty());

    EXPECT_EQ(t.find(1), _1);
    EXPECT_EQ(t.find(2), _2);
    EXPECT_EQ(t.find(3), _3);
    EXPECT_EQ(t.find(4), _4);
    EXPECT_EQ(t.find(5), _5);
    EXPECT_EQ(t.find(6), _6);
    EXPECT_EQ(t.find(7), _7);
    EXPECT_EQ(t.find(8), _8);
    EXPECT_EQ(t.find(9), _9);
    EXPECT_EQ(t.find(10), _10);
    EXPECT_EQ(t.find(11), _11);
    EXPECT_EQ(t.find(12), _12);
    EXPECT_EQ(t.find(13), _13);
}

TEST(Tree, stringKeysCreateFromMap)
{
    auto m = createItemMap();

    EXPECT_EQ(m.size(), 13);

    Er::Tree<ItemStringKey> t(m);
    using NodeT = Er::Tree<ItemStringKey>::Node;

    EXPECT_EQ(t.size(), 13);

    auto root = t.root();
    EXPECT_EQ(root->parent(), nullptr);
    EXPECT_EQ(root->data(), nullptr);

    auto& roots = root->children();
    EXPECT_EQ(roots.size(), 3);

    NodeT* _1 = nullptr;
    NodeT* _2 = nullptr;
    NodeT* _3 = nullptr;
    NodeT* _4 = nullptr;
    NodeT* _5 = nullptr;
    NodeT* _6 = nullptr;
    NodeT* _7 = nullptr;
    NodeT* _8 = nullptr;
    NodeT* _9 = nullptr;
    NodeT* _10 = nullptr;
    NodeT* _11 = nullptr;
    NodeT* _12 = nullptr;
    NodeT* _13 = nullptr;

    for (auto& i: roots)
    {
        if (i->data()->id == "1")
        {
            EXPECT_EQ(_1, nullptr);
            _1 = i;
        }
        else if (i->data()->id == "11")
        {
            EXPECT_EQ(_11, nullptr);
            _11 = i;
        }
        else if (i->data()->id == "13")
        {
            EXPECT_EQ(_13, nullptr);
            _13 = i;
        }
        else
        {
            EXPECT_EQ(true, false);
        }
    }

    EXPECT_NE(_1, nullptr);
    EXPECT_NE(_11, nullptr);
    EXPECT_NE(_13, nullptr);

    EXPECT_EQ(root->child(root->indexOfChild(_1)), _1);
    EXPECT_EQ(root->child(root->indexOfChild(_11)), _11);
    EXPECT_EQ(root->child(root->indexOfChild(_13)), _13);

    // non-existent child
    EXPECT_EQ(root->indexOfChild(root), NodeT::InvalidIndex);
    EXPECT_EQ(root->child(NodeT::InvalidIndex), nullptr);

    EXPECT_EQ(_1->parent(), root);
    EXPECT_NE(_1->data(), nullptr);
    EXPECT_STREQ(_1->data()->id.c_str(), "1");
    EXPECT_STREQ(_1->data()->parentId.c_str(), "1");
    EXPECT_STREQ(_1->data()->name.c_str(), "1");

    EXPECT_EQ(_11->parent(), root);
    EXPECT_NE(_11->data(), nullptr);
    EXPECT_STREQ(_11->data()->id.c_str(), "11");
    EXPECT_STREQ(_11->data()->parentId.c_str(), "11");
    EXPECT_STREQ(_11->data()->name.c_str(), "11");

    EXPECT_EQ(_13->parent(), root);
    EXPECT_NE(_13->data(), nullptr);
    EXPECT_STREQ(_13->data()->id.c_str(), "13");
    EXPECT_STREQ(_13->data()->parentId.c_str(), "13");
    EXPECT_STREQ(_13->data()->name.c_str(), "13");

    auto& _1_children = _1->children();
    EXPECT_EQ(_1_children.size(), 4);

    for (auto& i: _1_children)
    {
        if (i->data()->id == "2")
        {
            EXPECT_EQ(_2, nullptr);
            _2 = i;
        }
        else if (i->data()->id == "3")
        {
            EXPECT_EQ(_3, nullptr);
            _3 = i;
        }
        else if (i->data()->id == "5")
        {
            EXPECT_EQ(_5, nullptr);
            _5 = i;
        }
        else if (i->data()->id == "10")
        {
            EXPECT_EQ(_10, nullptr);
            _10 = i;
        }
        else
        {
            EXPECT_EQ(true, false);
        }
    }

    EXPECT_NE(_2, nullptr);
    EXPECT_NE(_3, nullptr);
    EXPECT_NE(_5, nullptr);
    EXPECT_NE(_10, nullptr);

    EXPECT_EQ(_1->child(_1->indexOfChild(_2)), _2);
    EXPECT_EQ(_1->child(_1->indexOfChild(_3)), _3);
    EXPECT_EQ(_1->child(_1->indexOfChild(_5)), _5);
    EXPECT_EQ(_1->child(_1->indexOfChild(_10)), _10);

    EXPECT_EQ(_2->parent(), _1);
    EXPECT_NE(_2->data(), nullptr);
    EXPECT_STREQ(_2->data()->id.c_str(), "2");
    EXPECT_STREQ(_2->data()->parentId.c_str(), "1");
    EXPECT_STREQ(_2->data()->name.c_str(), "2");

    EXPECT_EQ(_3->parent(), _1);
    EXPECT_NE(_3->data(), nullptr);
    EXPECT_STREQ(_3->data()->id.c_str(), "3");
    EXPECT_STREQ(_3->data()->parentId.c_str(), "1");
    EXPECT_STREQ(_3->data()->name.c_str(), "3");

    EXPECT_EQ(_5->parent(), _1);
    EXPECT_NE(_5->data(), nullptr);
    EXPECT_STREQ(_5->data()->id.c_str(), "5");
    EXPECT_STREQ(_5->data()->parentId.c_str(), "1");
    EXPECT_STREQ(_5->data()->name.c_str(), "5");

    EXPECT_EQ(_10->parent(), _1);
    EXPECT_NE(_10->data(), nullptr);
    EXPECT_STREQ(_10->data()->id.c_str(), "10");
    EXPECT_STREQ(_10->data()->parentId.c_str(), "1");
    EXPECT_STREQ(_10->data()->name.c_str(), "10");

    EXPECT_TRUE(_2->children().empty());

    auto& _3_children = _3->children();
    EXPECT_EQ(_3_children.size(), 1);

    for (auto& i: _3_children)
    {
        if (i->data()->id == "4")
        {
            EXPECT_EQ(_4, nullptr);
            _4 = i;
        }
        else
        {
            EXPECT_EQ(true, false);
        }
    }

    EXPECT_NE(_4, nullptr);
    EXPECT_EQ(_3->child(_3->indexOfChild(_4)), _4);
    EXPECT_EQ(_4->parent(), _3);
    EXPECT_NE(_4->data(), nullptr);
    EXPECT_STREQ(_4->data()->id.c_str(), "4");
    EXPECT_STREQ(_4->data()->parentId.c_str(), "3");
    EXPECT_STREQ(_4->data()->name.c_str(), "4");

    auto& _5_children = _5->children();
    EXPECT_EQ(_5_children.size(), 4);

    for (auto& i: _5_children)
    {
        if (i->data()->id == "6")
        {
            EXPECT_EQ(_6, nullptr);
            _6 = i;
        }
        else if (i->data()->id == "7")
        {
            EXPECT_EQ(_7, nullptr);
            _7 = i;
        }
        else if (i->data()->id == "8")
        {
            EXPECT_EQ(_8, nullptr);
            _8 = i;
        }
        else if (i->data()->id == "9")
        {
            EXPECT_EQ(_9, nullptr);
            _9 = i;
        }
        else
        {
            EXPECT_EQ(true, false);
        }
    }

    EXPECT_NE(_6, nullptr);
    EXPECT_EQ(_5->child(_5->indexOfChild(_6)), _6);
    EXPECT_EQ(_6->parent(), _5);
    EXPECT_NE(_6->data(), nullptr);
    EXPECT_STREQ(_6->data()->id.c_str(), "6");
    EXPECT_STREQ(_6->data()->parentId.c_str(), "5");
    EXPECT_STREQ(_6->data()->name.c_str(), "6");

    EXPECT_NE(_7, nullptr);
    EXPECT_EQ(_5->child(_5->indexOfChild(_7)), _7);
    EXPECT_EQ(_7->parent(), _5);
    EXPECT_NE(_7->data(), nullptr);
    EXPECT_STREQ(_7->data()->id.c_str(), "7");
    EXPECT_STREQ(_7->data()->parentId.c_str(), "5");
    EXPECT_STREQ(_7->data()->name.c_str(), "7");

    EXPECT_NE(_8, nullptr);
    EXPECT_EQ(_5->child(_5->indexOfChild(_8)), _8);
    EXPECT_EQ(_8->parent(), _5);
    EXPECT_NE(_8->data(), nullptr);
    EXPECT_STREQ(_8->data()->id.c_str(), "8");
    EXPECT_STREQ(_8->data()->parentId.c_str(), "5");
    EXPECT_STREQ(_8->data()->name.c_str(), "8");

    EXPECT_NE(_9, nullptr);
    EXPECT_EQ(_5->child(_5->indexOfChild(_9)), _9);
    EXPECT_EQ(_9->parent(), _5);
    EXPECT_NE(_9->data(), nullptr);
    EXPECT_STREQ(_9->data()->id.c_str(), "9");
    EXPECT_STREQ(_9->data()->parentId.c_str(), "5");
    EXPECT_STREQ(_9->data()->name.c_str(), "9");

    EXPECT_TRUE(_10->children().empty());

    auto& _11_children = _11->children();
    EXPECT_EQ(_11_children.size(), 1);

    for (auto& i: _11_children)
    {
        if (i->data()->id == "12")
        {
            EXPECT_EQ(_12, nullptr);
            _12 = i;
        }
        else
        {
            EXPECT_EQ(true, false);
        }
    }

    EXPECT_NE(_12, nullptr);
    EXPECT_EQ(_11->child(_11->indexOfChild(_12)), _12);
    EXPECT_EQ(_12->parent(), _11);
    EXPECT_NE(_12->data(), nullptr);
    EXPECT_STREQ(_12->data()->id.c_str(), "12");
    EXPECT_STREQ(_12->data()->parentId.c_str(), "11");
    EXPECT_STREQ(_12->data()->name.c_str(), "12");

    EXPECT_TRUE(_13->children().empty());

    EXPECT_EQ(t.find("1"), _1);
    EXPECT_EQ(t.find("2"), _2);
    EXPECT_EQ(t.find("3"), _3);
    EXPECT_EQ(t.find("4"), _4);
    EXPECT_EQ(t.find("5"), _5);
    EXPECT_EQ(t.find("6"), _6);
    EXPECT_EQ(t.find("7"), _7);
    EXPECT_EQ(t.find("8"), _8);
    EXPECT_EQ(t.find("9"), _9);
    EXPECT_EQ(t.find("10"), _10);
    EXPECT_EQ(t.find("11"), _11);
    EXPECT_EQ(t.find("12"), _12);
    EXPECT_EQ(t.find("13"), _13);
}


