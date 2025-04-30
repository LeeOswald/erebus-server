#pragma once

#include <erebus/rtl/rtl.hxx>

#include <string_view>


namespace Er
{


struct IUnknown
{
    static constexpr std::string_view IID = "Er.IUnknown";

    virtual void dispose() noexcept = 0;

    virtual void adopt(IUnknown* child) = 0;
    virtual void orphan(IUnknown* child) noexcept = 0;

    virtual IUnknown* queryInterface(std::string_view iid) noexcept = 0;

    template <class _Interface>
        requires std::is_base_of_v<IUnknown, _Interface>
    _Interface queryInterface() noexcept
    {
        return static_cast<_Interface*>(queryInterface(_Interface::IID));
    }

protected:
    virtual ~IUnknown() = default;
};


template <typename _Iface>
    requires requires(_Iface* p)
    {
        p->dispose();
    }
using DisposablePtr = std::unique_ptr<_Iface, decltype([](_Iface* p) { p->dispose(); })>;


} // namespace Er {}