#pragma once

#include <erebus/erebus.hxx>
#include <erebus-srv/plugin.hxx>

#include <vector>

namespace Er
{

namespace Private
{

struct ServerConfig final
{
    struct Endpoint final
    {
        std::string endpoint;
        bool ssl = false;
    };

    struct Plugin final
    {        
        std::string path;
        std::vector<Er::Server::PluginParams::Arg> args;
        bool enabled = true;
    };

    int verbose = 0;
    std::string logfile;
    int keeplogs = 3;
    std::string pidfile;
    std::vector<Endpoint> endpoints;
    std::string certificate;
    std::string privateKey;
    std::string rootCA;
    std::vector<Plugin> plugins;
};

ServerConfig loadConfig(const std::string& path);


} // namespace Private {}

} // namespace Er {}