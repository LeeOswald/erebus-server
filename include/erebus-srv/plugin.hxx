#pragma once

#include <erebus/log.hxx>
#include <erebus-srv/erebus-srv.hxx>


namespace Er
{

namespace Server
{

struct IServiceContainer;


struct PluginParams
{
    Er::Log::ILog* log = nullptr;
    IServiceContainer* container = nullptr;

    constexpr PluginParams() noexcept = default;

    constexpr PluginParams(Er::Log::ILog* log, IServiceContainer* container) noexcept
        : log(log)
        , container(container)
    {}
};


struct IPlugin
{
    virtual ~IPlugin() {}
};


typedef std::shared_ptr<IPlugin> (*CreatePluginFn)(const PluginParams&);



} // namespace Server {}
    
} // namespace Er {}    