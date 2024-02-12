#pragma once

#include <erebus/log.hxx>
#include <erebus-srv/erebus-srv.hxx>

#include <vector>

namespace Er
{

namespace Server
{

struct IServiceContainer;


struct PluginParams
{
    Er::Log::ILog* log = nullptr;
    std::vector<IServiceContainer*> containers;
};


struct IPlugin
{
    virtual ~IPlugin() {}
};


// the only symbol any plugin must export
typedef std::shared_ptr<IPlugin> (*createPlugin)(const PluginParams&);



} // namespace Server {}
    
} // namespace Er {}    