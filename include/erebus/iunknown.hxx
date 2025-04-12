#pragma once

#include <erebus/rtl/rtl.hxx>

#include <string_view>


namespace Er
{


struct IUnknown
{
    static constexpr std::string_view IID = "Er.IUnknown";

    virtual IUnknown* queryInterface(std::string iid) = 0;

    template <class Interface>
        requires std::is_base_of_v<IUnknown, Interface>
    Interface* queryInterface()
    {
        return static_cast<Interface*>(queryInterface(Interface::IID));
    }

protected:
    virtual ~IUnknown() = default;
};


} // namespace Er {}