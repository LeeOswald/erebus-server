#pragma once

#include <erebus/iunknown.hxx>
#include <erebus/rtl/property_bag.hxx>


namespace Er::Server
{


struct IPlugin
    : public IReferenceCounted
{
    static constexpr std::string_view IID = "Er.Server.IPlugin";

    virtual PropertyBag info() const = 0;

protected:
    virtual ~IPlugin() = default;
};


using PluginPtr = ReferenceCountedPtr<IPlugin>;


typedef PluginPtr(CreatePluginFn)(IUnknown* host, Log::LoggerPtr log, const PropertyMap& args);


namespace PluginProps
{

constexpr std::string_view Name{ "name" };
constexpr std::string_view Brief{ "brief" };
constexpr std::string_view VersionString{ "version_string" };

} // namespace PluginProps {}

} // namespace Er::Server {}


