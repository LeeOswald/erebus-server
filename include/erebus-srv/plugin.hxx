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


// the only symbols any plugin must export
typedef IPlugin* (createPlugin)(const PluginParams&);
typedef void (disposePlugin)(IPlugin*);



} // namespace Server {}
    
} // namespace Er {}    