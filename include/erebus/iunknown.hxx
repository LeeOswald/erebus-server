#pragma once

#include <erebus/rtl/rtl.hxx>

#include <chrono>
#include <string_view>


namespace Er
{

struct IUnknown
{
    static constexpr std::string_view IID = "Er.IUnknown";

    virtual IUnknown* queryInterface(std::string_view iid) noexcept = 0; // weak reference

    template <class _Interface>
        requires std::is_base_of_v<IUnknown, _Interface>
    _Interface* queryInterface() noexcept
    {
        return static_cast<_Interface*>(queryInterface(_Interface::IID));
    }

protected:
    virtual ~IUnknown() = default;
};


template <typename _Iface>
    requires std::is_base_of_v<IUnknown, _Iface>
struct IIDOf
{
    static constexpr std::string_view value = _Iface::IID;
};


struct IDisposable
    : public IUnknown
{
    static constexpr std::string_view IID = "Er.IDisposable";

    virtual void dispose() noexcept = 0;

protected:
    virtual ~IDisposable() = default;
};


struct IDisposableParent
    : public IUnknown
{
    static constexpr std::string_view IID = "Er.IDisposableParent";

    virtual void adopt(IDisposable* child) = 0;
    virtual void orphan(IDisposable* child) noexcept = 0;

protected:
    virtual ~IDisposableParent() = default;
};


template <typename _Iface>
    requires std::derived_from<_Iface, IDisposable>
using DisposablePtr = std::unique_ptr<_Iface, decltype([](_Iface* p)
{
    p->dispose();
})>;


struct IShared
    : public IUnknown
{
    virtual void addRef() noexcept = 0;
    virtual void release() noexcept = 0;
    
protected:
    virtual ~IShared() = default;
};


template <typename _Iface>
    requires std::derived_from<_Iface, IShared>
class SharedPtr
{
public:
    ~SharedPtr() noexcept
    {
        if (m_p)
            m_p->release();
    }

    constexpr SharedPtr() noexcept
        : m_p(nullptr)
    {
    }

    explicit constexpr SharedPtr(_Iface* p) noexcept
        : m_p(p)
    {
        // no addRef() here
    }

    SharedPtr(const SharedPtr& o) noexcept
        : SharedPtr(o.m_p)
    {
        if (m_p)
            m_p->addRef();
    }

    SharedPtr& operator=(const SharedPtr& o) noexcept
    {
        if (m_p != o.m_p)
        {
            SharedPtr tmp(o);
            swap(tmp);
        }

        return *this;
    }

    SharedPtr(SharedPtr&& o) noexcept
        : SharedPtr()
    {
        swap(o);
    }

    SharedPtr& operator=(SharedPtr&& o) noexcept
    {
        if (m_p != o.m_p)
        {
            SharedPtr tmp(std::move(o));
            swap(tmp);
        }

        return *this;
    }

    constexpr void swap(SharedPtr& o) noexcept
    {
        using std::swap;
        swap(m_p, o.m_p);
    }

    class NoReleaseIface
        : public _Iface
    {
    private:
        virtual void addRef() noexcept = 0;
        virtual void release() noexcept = 0;
    };

    NoReleaseIface* get() const noexcept
    {
        return static_cast<NoReleaseIface*>(m_p);
    }

    NoReleaseIface* operator->() const noexcept
    {
        ErAssert(m_p);

        return static_cast<NoReleaseIface*>(m_p);
    }

    operator bool() const noexcept
    {
        return !!m_p;
    }

    void reset() noexcept
    {
        if (m_p)
        {
            m_p->release();
            m_p = nullptr;
        }
    }

protected:
    _Iface* m_p;
};


struct IWaitable
    : public IUnknown
{
    static constexpr std::string_view IID = "Er.IWaitable";

    static constexpr std::uint32_t Infinite = std::uint32_t(-1);

    virtual bool wait(std::uint32_t milliseconds) noexcept = 0;

    template <class _Rep, class _Period>
    bool wait(std::chrono::duration<_Rep, _Period> timeout) noexcept
    {
        return wait(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count());
    }

protected:
    virtual ~IWaitable() = default;
};


} // namespace Er {}