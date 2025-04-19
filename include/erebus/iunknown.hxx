#pragma once

#include <erebus/rtl/rtl.hxx>

#include <string_view>


namespace Er
{


struct IUnknown
{
    static constexpr std::string_view IID = "Er.IUnknown";

    using Ptr = std::shared_ptr<IUnknown>;

    virtual ~IUnknown() = default;
    virtual Ptr queryInterface(std::string_view iid) noexcept = 0;

    template <class Interface>
        requires std::is_base_of_v<IUnknown, Interface>
    std::shared_ptr<Interface> queryInterface() noexcept
    {
        return std::static_pointer_cast<Interface>(queryInterface(Interface::IID));
    }
};


} // namespace Er {}