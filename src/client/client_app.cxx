#include "client_app.hxx"

#include <erebus/rtl/util/exception_util.hxx>
#include <erebus/rtl/util/file.hxx>

#include <iostream>


ClientApplication::ClientApplication() noexcept
    : Base(Base::EnableSignalHandler)
{
}

void ClientApplication::addCmdLineOptions(boost::program_options::options_description& options)
{
    options.add_options()
        ("connection", boost::program_options::value<std::string>(&m_cfgFile), "connection config file path")
        ("parallel,t", boost::program_options::value<unsigned>(&m_parallel)->default_value(1), "parallel thread count")
        ("count,n", boost::program_options::value<unsigned>(&m_iterations)->default_value(unsigned(-1)), "request repeat count")
        ("wait,w", boost::program_options::value<bool>(&m_wait)->default_value(true), "wait for current request completion before issuing another request")
        ("ping", boost::program_options::value<unsigned>(), "ping with specified number of bytes")
        ("sysinfo", boost::program_options::value<std::string>(), "query system properties with specified name pattern")
        ;
}

bool ClientApplication::loadConfiguration()
{
    try
    {
        if (m_cfgFile.empty())
        {
            std::cerr << "Configuration file name expected\n";
            return false;
        }

        auto configJson = Er::Util::loadTextFile(m_cfgFile);

        m_configRoot = Er::loadJson(configJson);
        if (m_configRoot.type() != Er::Property::Type::Map)
        {
            std::cerr << "Invalid configuration file format\n";
            return false;
        }
        m_config = m_configRoot.getMap();
        ErAssert(m_config);

        return true;
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return false;
}

bool ClientApplication::createChannel()
{
    ErAssert(m_config);

    Er::Util::ExceptionLogger xcptHandler(Er::Log::get());
    try
    {
        m_channel = Er::Ipc::Grpc::createChannel(*m_config);
    }
    catch (...)
    {
        Er::dispatchException(std::current_exception(), xcptHandler);

        return false;
    }

    return true;
}

bool ClientApplication::startTasks()
{
    if (m_parallel < 1)
    {
        ErLogError("At least one worker thread requred");
        return false;
    }

    if (args().contains("ping"))
    {
        auto payloadSize = args()["ping"].as<unsigned>();
        m_pingRunner.reset(new PingRunner([this]() { exitCondition().setAndNotifyOne(true); }, m_channel, m_parallel, m_wait, m_iterations, payloadSize));
        return true;
    }
    else if (args().contains("sysinfo"))
    {
        auto pattern = args()["sysinfo"].as<std::string>();
        m_systemInfoRunner.reset(new SystemInfoRunner([this]() { exitCondition().setAndNotifyOne(true); }, m_channel, m_parallel, m_iterations, m_wait, pattern));
        return true;
    }

    ErLogError("No tasks specified");
    return false;
}

int ClientApplication::run(int argc, char** argv)
{
    if (!createChannel())
        return EXIT_FAILURE;

    if (!startTasks())
        return EXIT_FAILURE;

    exitCondition().waitValue(true);

    if (signalReceived())
    {
        Er::Log::warning(Er::Log::get(), "Exiting due to signal {}", signalReceived());
    }

    m_pingRunner.reset();
    m_systemInfoRunner.reset();

    m_channel.reset();

    return EXIT_SUCCESS;
}