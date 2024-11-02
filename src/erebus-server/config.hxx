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
        std::string certificate;
        std::string privateKey;
        std::string rootCA;
    };

    struct Plugin final
    {        
        std::string path;
        std::vector<Er::Server::PluginParams::Arg> args;
        bool enabled = true;
    };

    std::string logFile;
    int keepLogs = 3;
    std::uint64_t maxLogSize = std::numeric_limits<std::uint64_t>::max();
    std::string pidFile;
    std::vector<Endpoint> endpoints;
    
    std::vector<Plugin> plugins;
};

ServerConfig loadConfig(const std::string& path);


} // namespace Private {}

} // namespace Er {}