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


struct IDisposableParent;

struct IDisposable
    : public IUnknown
{
    static constexpr std::string_view IID = "Er.IDisposable";

    virtual void dispose() noexcept = 0;
    virtual IDisposableParent* parent() const noexcept = 0;
    virtual void setParent(IDisposableParent* parent) noexcept = 0; // calls IDisposableParent.adopt()/orphan()

protected:
    virtual ~IDisposable() = default;
};


struct IDisposableParent
    : public IUnknown
{
    static constexpr std::string_view IID = "Er.IDisposableParent";

    virtual void adopt(IDisposable* child) = 0;                    // does NOT call IDisposable.setParent()
    virtual void orphan(IDisposable* child) noexcept = 0;          // does NOT call IDisposable.setParent()

protected:
    virtual ~IDisposableParent() = default;
};


struct DisposableDeleter final
{
    void operator()(IDisposable* p) noexcept
    {
        ErAssert(p);
        p->dispose();
    }
};

template <typename _Iface>
    requires std::derived_from<_Iface, IDisposable>
using DisposablePtr = std::unique_ptr<_Iface, DisposableDeleter>;


struct IReferenceCounted
    : public IUnknown
{
    virtual void addRef() noexcept = 0;
    virtual void release() noexcept = 0;
    
protected:
    virtual ~IReferenceCounted() = default;
};


template <typename _Iface>
    requires std::derived_from<_Iface, IReferenceCounted>
class ReferenceCountedPtr
{
public:
    using Type = _Iface;

    ~ReferenceCountedPtr() noexcept
    {
        if (m_p)
            m_p->release();
    }

    constexpr ReferenceCountedPtr() noexcept
        : m_p(nullptr)
    {
    }

    explicit constexpr ReferenceCountedPtr(_Iface* p) noexcept
        : m_p(p)
    {
        // no addRef() here
    }

    ReferenceCountedPtr(const ReferenceCountedPtr& o) noexcept
        : ReferenceCountedPtr(o.m_p)
    {
        if (m_p)
            m_p->addRef();
    }

    ReferenceCountedPtr& operator=(const ReferenceCountedPtr& o) noexcept
    {
        if (m_p != o.m_p)
        {
            ReferenceCountedPtr tmp(o);
            swap(tmp);
        }

        return *this;
    }

    ReferenceCountedPtr(ReferenceCountedPtr&& o) noexcept
        : ReferenceCountedPtr()
    {
        swap(o);
    }

    ReferenceCountedPtr& operator=(ReferenceCountedPtr&& o) noexcept
    {
        if (m_p != o.m_p)
        {
            ReferenceCountedPtr tmp(std::move(o));
            swap(tmp);
        }

        return *this;
    }

    constexpr void swap(ReferenceCountedPtr& o) noexcept
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

    template <class _Type>
    ReferenceCountedPtr<_Type> cast() noexcept
    {
        if (!m_p)
            return {};

        m_p->addRef();
        return ReferenceCountedPtr<_Type>(static_cast<_Type*>(m_p));
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