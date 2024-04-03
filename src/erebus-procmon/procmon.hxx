#pragma once

#include <erebus-srv/plugin.hxx>


#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef ER_PROCMON_EXPORTS
        #define ER_PROCMON_EXPORT __declspec(dllexport)
    #else
        #define ER_PROCMON_EXPORT __declspec(dllimport)
    #endif
#else
    #define ER_PROCMON_EXPORT __attribute__((visibility("default")))
#endif


extern "C"
{

ER_PROCMON_EXPORT Er::Server::IPlugin* createPlugin(const Er::Server::PluginParams& params);
ER_PROCMON_EXPORT void disposePlugin(Er::Server::IPlugin* plugin);

} // extern "C" {}


