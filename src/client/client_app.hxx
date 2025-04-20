#pragma once

#include <erebus/rtl/program.hxx>
#include <erebus/rtl/property_bag.hxx>
#include <erebus/ipc/grpc/grpc_client.hxx>

#include "system_info.hxx"


class ClientApplication
    : public Er::Program
{
    using Base = Er::Program;

public:
    ClientApplication() noexcept;

protected:
    std::string m_cfgFile;
    Er::Property m_configRoot;
    Er::PropertyMap const* m_config = nullptr;
    Er::Ipc::Grpc::ChannelPtr m_channel;
    unsigned m_parallel = 1;

    std::unique_ptr<PingRunner> m_pingRunner;

    void addCmdLineOptions(boost::program_options::options_description& options) override;
    bool loadConfiguration() override;
    bool createChannel();
    bool startTasks();

    int run(int argc, char** argv) override;
};