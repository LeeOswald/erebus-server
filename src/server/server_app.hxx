#pragma once


#include <erebus/rtl/program.hxx>

#include "config.hxx"


class ServerApplication
    : public Er::Program
{
    using Base = Er::Program;

public:
    ServerApplication() noexcept;

protected:
    std::string m_cfgFile;
    ServerConfig m_config;

    void addCmdLineOptions(boost::program_options::options_description& options) override;
    bool loadConfiguration() override;

    bool createPidfile();

    int run(int argc, char** argv) override;
};