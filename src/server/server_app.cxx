#include "server_app.hxx"

#include <erebus/ipc/grpc/private/grpc_server.hxx>
#include <erebus/rtl/logger/file_sink.hxx>
#include <erebus/rtl/logger/simple_formatter.hxx>
#include <erebus/rtl/system/user.hxx>
#include <erebus/rtl/util/exception_util.hxx>
#include <erebus/rtl/util/pid_file.hxx>

#include <iostream>


ServerApplication::ServerApplication() noexcept
    : Base(Base::CanBeDaemonized | Base::EnableSignalHandler)
{
}

void ServerApplication::addCmdLineOptions(boost::program_options::options_description& options)
{
    options.add_options()
        ("config", boost::program_options::value<std::string>(&m_cfgFile), "configuration file path")
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

        m_configRoot = Er::loadJson(m_cfgFile);
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

void ServerApplication::addLoggers(Er::Log::ITee* main)
{
    Base::addLoggers(main);
    
    ErAssert(m_config);
    auto prop = Er::findProperty(*m_config, "log_file", Er::Property::Type::String);
    if (!prop)
    {
        ErLogWarning2(Er::Log::get(), "No log file name specified");
        return;
    }

    auto logFileName = *prop->getString();

    unsigned logsToKeep = 3;
    prop = Er::findProperty(*m_config, "keep_logs", Er::Property::Type::Int64);
    if (prop)
    {
        logsToKeep = static_cast<unsigned>(*prop->getInt64());
    }

    std::uint64_t maxLogSize = 1024 * 1024 * 1024ULL;
    prop = Er::findProperty(*m_config, "max_log_size", Er::Property::Type::Int64);
    if (prop)
    {
        maxLogSize = static_cast<std::uint64_t>(*prop->getInt64());
    }

    auto formatOptions = Er::Log::SimpleFormatter::Options{
        Er::Log::SimpleFormatter::Option::Option::DateTime,
        Er::Log::SimpleFormatter::Option::Option::Level,
        Er::Log::SimpleFormatter::Option::Option::Tid,
        Er::Log::SimpleFormatter::Option::Option::TzLocal,
        Er::Log::SimpleFormatter::Option::Option::Lf,
        Er::Log::SimpleFormatter::Option::Option::Component
    };

    auto fileSink = Er::Log::makeFileSink(Er::ThreadSafe::No, logFileName, Er::Log::SimpleFormatter::make(formatOptions), logsToKeep, maxLogSize);
    main->addSink("file", fileSink);
}

bool ServerApplication::createPidfile()
{
    auto prop = Er::findProperty(*m_config, "pid_file", Er::Property::Type::String);
    if (!prop)
    {
        ErLogError2(Er::Log::get(), "No PID file name specified");
        return false;
    }

    auto pidFileName = *prop->getString();

    // create pidfile
    std::unique_ptr<Er::Util::PidFile> pidFile;
    if (!pidFileName.empty())
    {
        try
        {
            pidFile.reset(new Er::Util::PidFile(pidFileName));
        }
        catch (std::exception& e)
        {
            Er::Log::warning(Er::Log::get(), "Failed to create PID file {}: {}", pidFileName, e.what());
            auto existing = Er::Util::PidFile::read(pidFileName);
            if (existing)
                Er::Log::warning(Er::Log::get(), "Found running server instance with PID {}", *existing);

            return false;
        }

        Er::Log::info(Er::Log::get(), "Created PID file {}", pidFileName);
    }

    return true;
}

bool ServerApplication::createServer()
{
    ErAssert(m_config);

    auto prop = Er::findProperty(*m_config, "grpc_server", Er::Property::Type::Map);
    if (!prop)
    {
        Er::Log::error(Er::Log::get(), "No gRPC server settings found");
        return false;
    }

    auto settings = *prop->getMap();

    Er::Util::ExceptionLogger xcptHandler(Er::Log::get());
    try
    {
        auto serverLogger = Er::Log::makeSyncLogger("grpc_server");
        serverLogger->addSink("main", Er::Log::global());

        m_grpcServer = Er::Ipc::Grpc::createServer(settings, serverLogger);
    }
    catch (...)
    {
        Er::dispatchException(std::current_exception(), xcptHandler);

        return false;
    }

    return true;
}

int ServerApplication::run(int argc, char** argv)
{
    auto user = Er::System::User::current();
    Er::Log::info(Er::Log::get(), "Starting as user {}", user.name);

    if (!createPidfile())
        return EXIT_FAILURE;

    if (!createServer())
        return EXIT_FAILURE;

    ErLogInfo("Server started");

    exitCondition().waitValue(true);

    if (signalReceived())
    {
        Er::Log::warning(Er::Log::get(), "Exiting due to signal {}", signalReceived());
    }

    ErLogInfo("Server shutting down");

    m_grpcServer.reset();

    return EXIT_SUCCESS;
}