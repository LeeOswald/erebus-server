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
    struct Arg
    {
        std::string name;
        std::string value;

        template <typename KeyT, typename ValueT>
        Arg(KeyT&& key, ValueT&& val)
            : name(std::forward<KeyT>(key))
            , value(std::forward<ValueT>(val))
        {}
    };

    Er::Log::ILog* log = nullptr;
    std::vector<IServiceContainer*> containers;
    std::string binary;
    std::vector<Arg> args;
};


struct IPlugin
{
protected:
    virtual ~IPlugin() {}
};


// the only symbols any plugin must export
typedef IPlugin* (createPlugin)(const PluginParams&);
typedef void (disposePlugin)(IPlugin*);



} // namespace Server {}
    
} // namespace Er {}    