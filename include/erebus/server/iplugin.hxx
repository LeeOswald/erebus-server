#pragma once

#include <erebus/iunknown.hxx>
#include <erebus/rtl/property_bag.hxx>


namespace Er
{


struct IPlugin
    : public IUnknown
{
    static constexpr std::string_view IID = "Er.IPlugin";

    using Ptr = std::shared_ptr<IPlugin>;

    virtual ~IPlugin() = default;
    virtual PropertyBag info() = 0;
};


using CreatePluginFn = IPlugin::Ptr(*)(IUnknown::Ptr owner, Log::ILogger::Ptr log, const PropertyBag& args);

} // namespace Er {}


