#pragma once

#include <erebus/rtl/rtl.hxx>
#include <erebus/kv.hxx>



struct ServerConfig final
{
    struct Endpoint final
    {
        std::string endpoint;
        bool useTls = false;
        std::string certificate;
        std::string privateKey;
        std::string rootCertificates;
    };

    struct Plugin final
    {
        std::string path;
        Er::KvArray args;
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

