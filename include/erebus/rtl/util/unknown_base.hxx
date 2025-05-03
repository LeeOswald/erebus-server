#pragma once

#include <erebus/iunknown.hxx>
#include <erebus/rtl/mpl.hxx>

#include <atomic>
#include <mutex>
#include <unordered_map>

#include <erebus/rtl/log.hxx>


namespace Er::Util
{

namespace __
{

template <typename _Destination>
struct BindIsConvertibleTo
{
    template <typename _Source>
    using apply = std::is_convertible<_Source*, _Destination*>;
};

template <typename _Source>
struct BindIsConvertibleFrom
{
    template <typename _Destination>
    using apply = std::is_convertible<_Source*, _Destination*>;
};


template <typename... _I>
struct InheritInterfaces;

template <>
struct InheritInterfaces<> 
    : IUnknown
{
};

template <typename _I0, typename... _I>
struct InheritInterfaces<_I0, _I...> 
    : _I0, _I...
{
    static_assert(std::is_base_of<IUnknown, _I0>::value, "Interfaces must be derived from IUnknown");

    // resolve ambiguity for IUnknown methods
    virtual IUnknown* queryInterface(std::string_view iid) noexcept override = 0;
};


template <typename _Base, typename _QiMap>
struct ObjectRootImpl 
    : _Base
{
    // this type is exposed to user class
    using QiMap = _QiMap;
};


template <typename _Ty>
using GetQiMap = typename _Ty::QiMap;


template <typename... _Ty>
struct MakeObjectRoot
{
    using L = Mpl::List<_Ty...>;

    using InheritAll = Mpl::Inherit<_Ty...>;

    using DirectBases = Mpl::CopyIfQ<L, BindIsConvertibleFrom<InheritAll>>;
    using IndirectBases = Mpl::RemoveIfQ<L, BindIsConvertibleFrom<InheritAll>>;

    using InheritDirectBases = Mpl::Apply<InheritInterfaces, DirectBases>;

    // find first direct base, which is derived from specified indirect base
    // save found pair<_Base, Derived> into QueryInterface map
    template <typename U>
    struct MapToAppropriateDirectBaseImpl
    {
        using index = Mpl::FindIfQ<DirectBases, BindIsConvertibleTo<U>>;
        using type = Mpl::List<U, Mpl::Get<index, DirectBases>>;
    };

    template <typename U>
    using MapToAppropriateDirectBase = typename MapToAppropriateDirectBaseImpl<U>::type;

    template <typename U>
    using MapToFinalType = Mpl::List<U, InheritDirectBases>;

    using QiMap = Mpl::Append<
        // map every indirect base to appropriate direct base
        Mpl::Transform<MapToAppropriateDirectBase, IndirectBases>,
        // map every direct base to final type
        Mpl::Transform<MapToFinalType, DirectBases>
    >;

    using type = ObjectRootImpl<InheritDirectBases, QiMap>;
};


template <>
struct MakeObjectRoot<>
{
    using type = ObjectRootImpl<InheritInterfaces<>, Mpl::List<>>;
};

} // namespace __ {}


template <typename... _Ty>
struct Interfaces 
    : __::MakeObjectRoot<_Ty...>::type 
{
};


namespace __
{

class SimpleObjectBaseRoot
{
public:
    IUnknown* internalQueryInterfaceImpl(std::string_view iid) noexcept
    {
        return nullptr;
    }
};


template <typename _Ty>
struct IFaceWrapper 
    : _Ty
{
    typedef _Ty IFace;
};


template <typename _IfaceWrapper, typename _Base>
class SimpleObjectBaseInterfaceImpl 
    : public _IfaceWrapper
    , public _Base
{
public:
    IUnknown* internalQueryInterfaceImpl(std::string_view iid) noexcept
    {
        if (iid == IIDOf<typename _IfaceWrapper::IFace>::value)
        {
            return static_cast<_IfaceWrapper*>(this);
        }

        return this->_Base::internalQueryInterfaceImpl(iid);
    }

    IUnknown* getControllingIUnknown() noexcept
    {
        return static_cast<IUnknown*>(static_cast<_IfaceWrapper*>(this));
    }
};


template <typename _Ifaces>
class ObjectBaseInterfaces 
    : public Mpl::RightFold<Mpl::Transform<IFaceWrapper, _Ifaces>, SimpleObjectBaseRoot, SimpleObjectBaseInterfaceImpl>
{
};


struct ObjectBaseTag {};


template <typename Ifaces>
class ObjectBaseImpl 
    : public ObjectBaseInterfaces<Ifaces>
    , public ObjectBaseTag
{
public:
    virtual ~ObjectBaseImpl() = default;

    ObjectBaseImpl() noexcept = default;

    IUnknown* internalQueryInterface(std::string_view iid) noexcept
    {
        if (iid == IIDOf<IUnknown>::value)
            return this->getControllingIUnknown();

        return this->internalQueryInterfaceImpl(iid);
    }

    IUnknown* queryInterface(std::string_view iid) noexcept
    {
        return internalQueryInterface(iid);
    }
};


} // namespace __ {}


template <typename... _Ifaces>
class ObjectBase 
    : public __::ObjectBaseImpl<Mpl::List<_Ifaces...>>
{
public:
    using Base = __::ObjectBaseImpl<Mpl::List<_Ifaces...>>;
    using ObjectRoot = ObjectBase<_Ifaces...>;

    ObjectBase() noexcept = default;

    ObjectBase(const ObjectBase&) = delete;
    ObjectBase& operator=(const ObjectBase&) = delete;

    ObjectBase(ObjectRoot&&) = delete;
    ObjectBase& operator=(ObjectBase&&) = delete;
};



template <class _Base>
struct DisposableBase
    : public _Base
{
    ~DisposableBase()
    {
        if (m_owner)
        {
            m_owner->orphan(this);
        }
    }

    template <typename... _Args>
    explicit DisposableBase(IDisposableParent* owner, _Args... args)
        : _Base(std::forward<_Args>(args)...)
        , m_owner(owner)
    {
        if (m_owner)
        {
            m_owner->adopt(this);
        }
    }

    void dispose() noexcept override
    {
        delete this;
    }

    IDisposableParent* parent() const noexcept
    {
        return m_owner;
    }

    void setParent(IDisposableParent* parent) noexcept
    {
        if (m_owner)
            m_owner->orphan(this);

        m_owner = nullptr;

        if (parent)
        {
            parent->adopt(this);
            m_owner = parent;
        }
    }

private:
    IDisposableParent* m_owner;
};



template <class _Base>
struct DisposableParentBase
    : public _Base
{
    ~DisposableParentBase()
    {
        m_destructing = true;
        m_children.clear();
    }

    template <typename... _Args>
    DisposableParentBase(_Args... args)
        : _Base(std::forward<_Args>(args)...)
    {
    }

    void adopt(IDisposable* child) override
    {
        ErAssert(child);
        ErAssert(!child->parent());
        ErAssert(!m_destructing);

        std::lock_guard l(m_mutex);

        ErAssert(m_children.find(child) == m_children.end());
        m_children.insert({ child, DisposablePtr<IDisposable>{ child } });
    }

    void orphan(IDisposable* child) noexcept override
    {
        if (m_destructing)
            return;

        ErAssert(child);
        ErAssert(child->parent() == this);

        std::lock_guard l(m_mutex);
        
        auto it = m_children.find(child);
        if (it != m_children.end())
        {
            it->second.release(); // do not dispose() here
            m_children.erase(child);
        }
    }

private:
    std::mutex m_mutex;
    std::unordered_map<IUnknown*, DisposablePtr<IDisposable>> m_children;
    bool m_destructing = false;
};


template <class _Base>
struct SharedBase
    : public _Base
{
    ~SharedBase() = default;

    template <typename... _Args>
    explicit SharedBase(_Args... args)
        : _Base(std::forward<_Args>(args)...)
        , m_refs(1)
    {
    }

    void addRef() noexcept override
    {
        ++m_refs;
    }

    void release() noexcept override
    {
        if (m_refs.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            delete this;
        }
    }

private:
    std::atomic<std::size_t> m_refs;
};

} // namespace Er::Util {}