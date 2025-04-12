#include "server_app.hxx"

#include <erebus/rtl/system/user.hxx>
#include <erebus/rtl/util/pid_file.hxx>

#include <iostream>


ServerApplication::ServerApplication() noexcept
    : Base(Base::CanBeDaemonized | Base::EnableSignalHandler)
{
}

void ServerApplication::addCmdLineOptions(boost::program_options::options_description& options)
{
    options.add_options()
        ("config", boost::program_options::value<std::string>(&m_cfgFile)->default_value("erebus-server.cfg"), "configuration file path")
#if ER_LINUX
        ("noroot", "don't require root privileges")
#endif
        ;
}

bool ServerApplication::loadConfiguration()
{
    try
    {
        if (m_cfgFile.empty())
        {
            std::cerr << "Configuration file name expected\n";
            return false;
        }

        m_config = ::loadConfig(m_cfgFile);
        return true;
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to load the configuration: " << e.what() << std::endl;
    }

    return false;
}

bool ServerApplication::createPidfile()
{
    // create pidfile
    std::unique_ptr<Er::Util::PidFile> pidFile;
    if (!m_config.pidFile.empty())
    {
        try
        {
            pidFile.reset(new Er::Util::PidFile(m_config.pidFile));
        }
        catch (std::exception& e)
        {
            Er::Log::warning(Er::Log::get(), "Failed to create PID file {}: {}", m_config.pidFile, e.what());
            auto existing = Er::Util::PidFile::read(m_config.pidFile);
            if (existing)
                Er::Log::warning(Er::Log::get(), "Found running server instance with PID {}", *existing);

            return false;
        }

        Er::Log::info(Er::Log::get(), "Created PID file {}", m_config.pidFile);
    }

    return true;
}

int ServerApplication::run(int argc, char** argv)
{
    auto user = Er::System::User::current();
    Er::Log::info(Er::Log::get(), "Starting as user {}", user.name);

    if (!createPidfile())
        return EXIT_FAILURE;

    ErLogInfo("Server started");

    exitCondition().waitValue(true);

    if (signalReceived())
    {
        Er::Log::warning(Er::Log::get(), "Exiting due to signal {}", signalReceived());
    }

    ErLogInfo("Server shutting down");

    return EXIT_SUCCESS;
}