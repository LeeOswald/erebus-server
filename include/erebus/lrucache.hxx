#pragma once

#include <erebus/erebus.hxx>

#include <list>
#include <unordered_map>


namespace Er
{

template <typename KeyT, typename ValueT>
class LruCache
    : public Er::NonCopyable
{
public:
    using Key = KeyT;
    using Value = ValueT;

    LruCache(size_t limit)
        : m_limit(limit)
    {
    }

    template <typename ValueType>
    void put(const Key& key, ValueType&& value)
    {
        // always put new items to the front
        m_list.emplace_front(std::make_pair(key, std::forward<ValueType>(value)));

        // if it is already there, remove the copy
        auto it = m_map.find(key);
        if (it != m_map.end())
        {
            m_list.erase(it->second);
            m_map.erase(it);
        }

        m_map[key] = m_list.begin();

        // remove oldest items
        if (m_map.size() > m_limit)
        {
            auto last = m_list.end();
            last--;
            m_map.erase(last->first);
            m_list.pop_back();
        }
    }

    const Value* get(const Key& key) const
    {
        auto it = m_map.find(key);
        if (it == m_map.end())
        {
            return nullptr;
        }

        // move requested item to the front
        m_list.splice(m_list.begin(), m_list, it->second);
        return &it->second->second;
    }

    Value* get(const Key& key)
    {
        auto it = m_map.find(key);
        if (it == m_map.end())
        {
            return nullptr;
        }

        // move requested item to the front
        m_list.splice(m_list.begin(), m_list, it->second);
        return &it->second->second;
    }

    bool exists(const Key& key) const
    {
        return m_map.find(key) != m_map.end();
    }

    size_t size() const
    {
        return m_map.size();
    }

private:
    using List = std::list<std::pair<Key, Value>>;

    mutable List m_list;
    std::unordered_map<Key, typename List::iterator> m_map;
    size_t m_limit;
};



} // namespace Er {}