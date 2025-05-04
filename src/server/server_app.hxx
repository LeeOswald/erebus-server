#pragma once


#include <erebus/ipc/grpc/server/iserver.hxx>
#include <erebus/rtl/program.hxx>
#include <erebus/rtl/property_bag.hxx>
#include <erebus/server/iplugin_host.hxx>

#include <vector>


class ServerApplication final
    : public Er::Program
{
    using Base = Er::Program;

public:
    ServerApplication() noexcept;

private:
    void addCmdLineOptions(boost::program_options::options_description& options) override;
    bool loadConfiguration() override;
    void addLoggers(Er::Log::ITee* main) override;

    bool createPidfile();
    bool createServer();
    bool startServer();
    bool loadPlugins();

    int run(int argc, char** argv) override;

    std::string m_cfgFile;
    Er::Property m_configRoot;
    Er::PropertyMap const* m_config = nullptr;
    Er::Ipc::Grpc::ServerPtr m_grpcServer;
    Er::Server::PluginHostPtr m_pluginMgr;
    std::vector<Er::Server::PluginPtr> m_plugins;
};
