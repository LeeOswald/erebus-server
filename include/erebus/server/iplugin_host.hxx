#pragma once

#include <erebus/ipc/grpc/server/iserver.hxx>
#include <erebus/server/iplugin.hxx>


namespace Er::Server
{

struct IPluginHost
    : public IReferenceCounted
{
    static constexpr std::string_view IID = "Er.IPluginHost";

    virtual PluginPtr loadPlugin(const std::string& path, const PropertyMap& args) = 0;
    virtual Ipc::Grpc::IServer* server() noexcept = 0;

protected:
    virtual ~IPluginHost() = default;
};

using PluginHostPtr = ReferenceCountedPtr<IPluginHost>;


} // namespace Er::Server {}