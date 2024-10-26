#pragma once

#include <erebus/empty.hxx>
#include <erebus/exception.hxx>
#include <erebus/util/ptrutils.hxx>

#include <unordered_map>


namespace Er
{

//    
// Tree with insertion/removal hooks
//    
    
template <class T>
concept Treeable =
    requires(T instance, const T const_instance)
    {
        requires std::equality_comparable<typename T::Key>;
        { const_instance.key() } -> std::same_as<const typename T::Key&>;
        { const_instance.parentKey() } -> std::same_as<const typename T::Key&>;
        { instance.parentKey() } -> std::same_as<typename T::Key&>;
        { const_instance.isRoot() } -> std::convertible_to<bool>;
        { instance.context() } -> std::same_as<void*&>;
        { const_instance.context() } -> std::same_as<void* const&>;
    };



template <class ItemT, class ItemPtrT = std::shared_ptr<ItemT>, class NodeDataT = Empty>
    requires Treeable<ItemT> && Util::IsSharedPtrType<ItemPtrT>::value
class Tree final
    : public NonCopyable
{
public:
    using Item = ItemT;
    using ItemPtr = ItemPtrT;
    using Key = typename Item::Key;

    struct Node final
        : public NodeDataT
        , public NonCopyable
    {
        static constexpr const size_t InvalidIndex = size_t(-1);

        ~Node()
        {
            if (m_data)
                m_data->context() = nullptr;
        }

        constexpr Node() = default;

        constexpr explicit Node(ItemPtr data)
            : m_data(data)
        {
            ErAssert(data);
            m_data->context() = this;
        }

        const Item* data() const noexcept
        {
            return Util::getPlainPtr(m_data);
        }

        Item* data() noexcept
        {
            return Util::getPlainPtr(m_data);
        }

        const Node* parent() const noexcept
        {
            return m_parent;
        }

        Node* parent() noexcept
        {
            return m_parent;
        }

        const std::vector<Node*>& children() const noexcept
        {
            return m_children;
        }

        std::vector<Node*>& children() noexcept
        {
            return m_children;
        }

        size_t indexOfChild(const Node* child) const noexcept
        {
            auto it = std::find(m_children.begin(), m_children.end(), child);
            return (it != m_children.end()) ? std::distance(m_children.begin(), it) : InvalidIndex;
        }

        Node* child(size_t index) noexcept
        {
            return (index < m_children.size() ? m_children[index] : nullptr);
        }

        const Node* child(size_t index) const noexcept
        {
            return (index < m_children.size() ? m_children[index] : nullptr);
        }

    private:
        friend class Tree;

        ItemPtr m_data;
        Node* m_parent = nullptr;
        std::vector<Node*> m_children;
    };

    constexpr Tree() = default;

    template <class ContainerT>
    explicit Tree(const ContainerT& source)
    {
        for (auto sourceIt = source.begin(); sourceIt != source.end(); ++sourceIt)
        {
            auto dataPtr = getItem<ContainerT>(sourceIt);
            auto& key = dataPtr->key();

            ErAssert(dataPtr);

            auto r = m_nodes.emplace(key, std::make_unique<Node>(dataPtr));
            if (!r.second)
                throw Exception(ER_HERE(), "Duplicate tree item key");
        }

        ErAssert(m_nodes.size() == source.size());

        // look for parents and children
        for (auto nodeIt = m_nodes.begin(); nodeIt != m_nodes.end(); ++nodeIt)
        {
            auto& key = nodeIt->first;
            auto& nodePtr = nodeIt->second;
            ErAssert(nodePtr);
            auto dataPtr = nodePtr->m_data;
            ErAssert(dataPtr);
            auto parentIt = m_nodes.find(dataPtr->parentKey());
            if ((parentIt != m_nodes.end()) && !dataPtr->isRoot())
            {
                auto& parentNodePtr = parentIt->second;
                ErAssert(parentNodePtr);

                nodePtr->m_parent = Util::getPlainPtr(parentNodePtr);
                parentNodePtr->m_children.push_back(Util::getPlainPtr(nodePtr));
            }
            else
            {
                nodePtr->m_parent = &m_root;
                m_root.m_children.push_back(Util::getPlainPtr(nodePtr));
            }
        }
    }

    constexpr const Node* root() const noexcept
    {
        return &m_root;
    }

    constexpr bool empty() const noexcept
    {
        return m_nodes.empty();
    }

    constexpr size_t size() const noexcept
    {
        return m_nodes.size();
    }

    const Node* find(const Key& key) const noexcept
    {
        auto it = m_nodes.find(key);
        return (it != m_nodes.end() ? it->second.get() : nullptr);
    }

    Node* insert(ItemPtr item)
    {
        auto beginInsert = [](Node* node, Node* parent, size_t index){};
        auto endInsert = [](){};
        auto beginMove = [](Node* node, Node* oldParent, size_t oldIndex, Node* newParent, size_t newIndex){};
        auto endMove = [](){};

        return insert(item, beginInsert, endInsert, beginMove, endMove);
    }

    template <typename BeginInsertF, typename EndInsertF, typename BeginMoveF, typename EndMoveF>
        requires std::is_invocable_v<BeginInsertF, Node*, Node*, size_t> &&
                std::is_invocable_v<EndInsertF> &&
                std::is_invocable_v<BeginMoveF, Node*, Node*, size_t, Node*, size_t> &&
                std::is_invocable_v<EndMoveF>
    Node* insert(ItemPtr item, BeginInsertF beginInsert, EndInsertF endInsert, BeginMoveF beginMove, EndMoveF endMove)
    {
        auto nodePtr = std::make_unique<Node>(item);
        auto node = nodePtr.get();
        auto r = m_nodes.emplace(item->key(), std::move(nodePtr));
        if (!r.second)
            throw Exception(ER_HERE(), "Duplicate tree item key");

        // find & assign parent
        if (item->isRoot())
        {
            beginInsert(node, &m_root, m_root.m_children.size());

            m_root.m_children.push_back(node);
            node->m_parent = &m_root;

            endInsert();
        }
        else
        {
            auto parentIt = m_nodes.find(item->parentKey());
            if (parentIt == m_nodes.end())
            {
                beginInsert(node, &m_root, m_root.m_children.size());

                m_root.m_children.push_back(node);
                node->m_parent = &m_root;

                endInsert();
            }
            else
            {
                auto& parentNodePtr = parentIt->second;
                ErAssert(parentNodePtr);

                beginInsert(node, Util::getPlainPtr(parentNodePtr), parentNodePtr->m_children.size());

                parentNodePtr->m_children.push_back(node);
                node->m_parent = Util::getPlainPtr(parentNodePtr);

                endInsert();
            }
        }

        // check for items that miss their parents
        for (auto it = m_root.m_children.begin(); it != m_root.m_children.end();)
        {
            auto parentlessNode = *it;
            if (parentlessNode != node)
            {
                if (parentlessNode->data()->parentKey() == item->key())
                {
                    beginMove(node, &m_root, std::distance(m_root.m_children.begin(), it), node, node->m_children.size());

                    parentlessNode->m_parent = node;
                    node->m_children.push_back(parentlessNode);
                    it = m_root.m_children.erase(it);

                    endMove();
                }
                else
                {
                    ++it;
                }
            }
            else
            {
                ++it;
            }
        }

        return node;
    }

    void remove(const Item* item)
    {
        auto beginRemove = [](Node* node, Node* parent, size_t index){};
        auto endRemove = [](){};
        auto beginMove = [](Node* node, Node* oldParent, size_t oldIndex, Node* newParent, size_t newIndex){};
        auto endMove = [](){};

        remove(item, beginRemove, endRemove, beginMove, endMove);
    }

    template <typename BeginRemoveF, typename EndRemoveF, typename BeginMoveF, typename EndMoveF>
        requires std::is_invocable_v<BeginRemoveF, Node*, Node*, size_t> &&
            std::is_invocable_v<EndRemoveF> &&
            std::is_invocable_v<BeginMoveF, Node*, Node*, size_t, Node*, size_t> &&
            std::is_invocable_v<EndMoveF>
    void remove(const Item* item, BeginRemoveF beginRemove, EndRemoveF endRemove, BeginMoveF beginMove, EndMoveF endMove)
    {
        auto it = m_nodes.find(item->key());
        if (it == m_nodes.end())
            return;

        auto node = it->second.get();
        ErAssert(node != &m_root);

        // root adopts this node's children
        for (auto childIt = node->m_children.begin(); childIt != node->m_children.end(); ++childIt)
        {
            beginMove(*childIt, node, std::distance(node->m_children.begin(), childIt), &m_root, m_root.m_children.size());

            m_root.m_children.push_back(*childIt);
            (*childIt)->m_parent = &m_root;

            endMove();
        }

        // orphan this node
        auto parent = node->m_parent;
        ErAssert(parent);
        auto parentIt = std::find(parent->m_children.begin(), parent->m_children.end(), node);
        ErAssert(parentIt != parent->m_children.end());

        beginRemove(node, parent, std::distance(parent->m_children.begin(), parentIt));

        parent->m_children.erase(parentIt);

        endRemove();

        // finally remove this node
        m_nodes.erase(it);
    }

private:
    template <class ContainerT>
    using ValueType = typename ContainerT::value_type;

    template <class ContainerT>
    static const auto& getItem(const typename ContainerT::const_iterator& it)
    {
        if constexpr (requires(const ContainerT& c) { c.begin()->second; })
        {
            // for associative containers
            return it->second;
        }
        else
        {
            // for non-associative containers
            return *it;    
        }
    }

    std::unordered_map<Key, std::unique_ptr<Node>> m_nodes;
    Node m_root; // fake root: even if a node has no parent it gets adopted here
};
    
    
    
} // namespace Er {}