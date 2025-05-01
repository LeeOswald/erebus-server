#pragma once

#include <erebus/rtl/rtl.hxx>

#include <string_view>


namespace Er
{

struct IUnknown
{
    static constexpr std::string_view IID = "Er.IUnknown";

    virtual IUnknown* queryInterface(std::string_view iid) noexcept = 0;

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


} // namespace Er {}