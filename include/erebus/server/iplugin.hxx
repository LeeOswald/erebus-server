#pragma once

#include <erebus/iunknown.hxx>
#include <erebus/rtl/property_bag.hxx>


namespace Er
{


struct IPlugin
    : public IUnknown
{
    static constexpr std::string_view IID = "Er.IPlugin";

    virtual PropertyBag info() = 0;

protected:
    virtual ~IPlugin() = default;
};


using CreatePluginFn = IPlugin*(*)(IUnknown* owner, Log::ILogger::Ptr log, const PropertyBag& args);

} // namespace Er {}


