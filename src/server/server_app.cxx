#include "server_app.hxx"

#include <erebus/ipc/grpc/server/grpc_server.hxx>
#include <erebus/rtl/logger/file_sink.hxx>
#include <erebus/rtl/logger/simple_formatter.hxx>
#include <erebus/rtl/system/user.hxx>
#include <erebus/rtl/util/exception_util.hxx>
#include <erebus/rtl/util/file.hxx>
#include <erebus/rtl/util/pid_file.hxx>
#include <erebus/server/plugin_mgr.hxx>

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

        auto configJson = Er::Util::loadFile(m_cfgFile).release();

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
        maxLogSize = 1024 * 1024 * static_cast<std::uint64_t>(*prop->getInt64());
        maxLogSize = std::max(maxLogSize, std::uint64_t(1024));
    }

    auto formatOptions = Er::Log::SimpleFormatter::Options{
        Er::Log::SimpleFormatter::Option::Option::DateTime,
        Er::Log::SimpleFormatter::Option::Option::Level,
        Er::Log::SimpleFormatter::Option::Option::Tid,
        Er::Log::SimpleFormatter::Option::Option::TzLocal,
        Er::Log::SimpleFormatter::Option::Option::Lf,
        Er::Log::SimpleFormatter::Option::Option::Component
    };

    auto fileSink = Er::Log::makeFileSink(Er::ThreadSafe::No, logFileName, Er::Log::makeSimpleFormatter(formatOptions), logsToKeep, maxLogSize);
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
        serverLogger->addSink("main", Er::Log::global().cast<Er::Log::ISink>());
        m_grpcServer = Er::Ipc::Grpc::createServer(settings, serverLogger);

        auto sysInfoLogger = Er::Log::makeSyncLogger("system_info");
        sysInfoLogger->addSink("main", Er::Log::global().cast<Er::Log::ISink>());
        auto sysInfoService = Er::Ipc::Grpc::createSystemInfoService(sysInfoLogger);
        
        m_grpcServer->addService(std::move(sysInfoService));
    }
    catch (...)
    {
        Er::dispatchException(std::current_exception(), xcptHandler);

        return false;
    }

    return true;
}

bool ServerApplication::loadPlugins()
{
    ErAssert(m_config);

    auto prop = Er::findProperty(*m_config, "plugins", Er::Property::Type::Vector);
    if (!prop)
    {
        Er::Log::warning(Er::Log::get(), "No plugin settings found");
        return true; // run even w/out plugins
    }

    auto settings = *prop->getVector();
    if (settings.empty())
    {
        Er::Log::warning(Er::Log::get(), "No plugins to load");
        return true;
    }

    m_pluginMgr.reset(new Er::Server::PluginMgr(Er::Log::global()));

    for (auto& entry : settings)
    {
        auto m = entry.getMap();
        if (!m)
        {
            Er::Log::error(Er::Log::get(), "Plugin settings entry is not an object");
            continue;
        }

        auto path = Er::findProperty(*m, "path", Er::Property::Type::String);
        if (!path)
        {
            Er::Log::error(Er::Log::get(), "Plugin path expected");
            continue;
        }

        const Er::PropertyMap* pluginArgs = nullptr;
        auto args = Er::findProperty(*m, "args", Er::Property::Type::Map);
        if (args)
            pluginArgs = args->getMap();


        Er::Util::ExceptionLogger xcptHandler(Er::Log::get());
        try
        {
            auto plugin = m_pluginMgr->loadPlugin(*path->getString(), *pluginArgs);

            auto props = plugin->info();

            Er::Log::AtomicBlock a(Er::Log::get());

            ErLogIndent2(Er::Log::get(), Er::Log::Level::Info, "Loaded plugin {}", *path->getString());
            for (auto& prop : props)
            {
                ErLogInfo2(Er::Log::get(), "{}: {}", prop.name(), prop.str());
            }

            m_plugins.push_back(plugin);
        }
        catch (...)
        {
            Er::dispatchException(std::current_exception(), xcptHandler);

            Er::Log::error(Er::Log::get(), "Failed to load plugin from [{}]", *path->getString());
        }
    }

    return true;

}

bool ServerApplication::startServer()
{
    Er::Util::ExceptionLogger xcptHandler(Er::Log::get());
    try
    {
        m_grpcServer->start();
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

    if (!loadPlugins())
        return EXIT_FAILURE;

    if (!startServer())
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