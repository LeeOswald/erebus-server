#pragma once

#include <erebus/erebus.hxx>

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
        std::vector<std::string> args;
    };

    int verbose = 0;
    std::string logfile;
    std::vector<Endpoint> endpoints;
    std::string certificate;
    std::string privateKey;
    std::string rootCA;
    std::string userDb;
    std::vector<Plugin> plugins;
};

ServerConfig loadConfig(const std::string& path);


} // namespace Private {}

} // namespace Er {}