#pragma once


#include <erebus/ipc/grpc/server/iserver.hxx>
#include <erebus/rtl/program.hxx>
#include <erebus/rtl/property_bag.hxx>


class ServerApplication
    : public Er::Program
{
    using Base = Er::Program;

public:
    ServerApplication() noexcept;

protected:
    std::string m_cfgFile;
    Er::Property m_configRoot;
    Er::PropertyMap const* m_config = nullptr;
    Er::DisposablePtr<Er::Ipc::Grpc::IServer> m_grpcServer;

    void addCmdLineOptions(boost::program_options::options_description& options) override;
    bool loadConfiguration() override;
    void addLoggers(Er::Log::ITee* main) override;

    bool createPidfile();
    bool createServer();
    bool startServer();

    int run(int argc, char** argv) override;
};