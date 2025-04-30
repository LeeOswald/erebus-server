#pragma once

#include <erebus/iunknown.hxx>

#include <mutex>
#include <unordered_map>


namespace Er::Util
{

template <class _Mutex, class... _Ifaces>
struct UnknownBase
    : public _Ifaces...
{
    ~UnknownBase()
    {
        if (m_owner)
            m_owner->orphan(this);
    }

    explicit UnknownBase(IUnknown* owner) noexcept
        : m_owner(owner)
    {
        if (owner)
            owner->adopt(this);
    }

    void dispose() noexcept override
    {
        delete this;
    }

    void adopt(IUnknown* child) override
    {
        ErAssert(child != static_cast<IUnknown*>(this));

        std::lock_guard l(m_mutex);

        ErAssert(m_children.find(child) == m_children.end());
        m_children.insert({ child, ChildPtr{ child } });
    }

    void orphan(IUnknown* child) noexcept override
    {
        ErAssert(child != static_cast<IUnknown*>(this));

        std::lock_guard l(m_mutex);
        
        auto it = m_children.find(child);
        if (it != m_children.end())
        {
            it->second.release(); // do not dispose() here
            m_children.erase(child);
        }
    }

    using ChildPtr = DisposablePtr<IUnknown>;

    IUnknown* m_owner;
    _Mutex m_mutex;
    std::unordered_map<IUnknown*, ChildPtr> m_children;
};


} // namespace Er::Util {}