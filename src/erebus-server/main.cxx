#include <boost/program_options.hpp>
#include <boost/stacktrace.hpp>

#include <erebus/condition.hxx>
#include <erebus/knownprops.hxx>
#include <erebus/system/process.hxx>
#include <erebus/system/thread.hxx>
#include <erebus/system/user.hxx>
#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/file.hxx>
#include <erebus/util/format.hxx>
#include <erebus/util/pidfile.hxx>
#include <erebus/util/sha256.hxx>
#if ER_POSIX
    #include <erebus/util/signalhandler.hxx>
#endif

#include <erebus-srv/erebus-srv.hxx>

#include "config.hxx"
#include "coreservice.hxx"
#include "logger.hxx"
#include "pluginmgr.hxx"

#include <filesystem>
#include <future>
#include <iostream>
#include <syncstream>
#include <vector>


namespace
{

Er::Log::ILog* g_log = nullptr;
Er::Event g_exitCondition(false);
std::optional<int> g_signalReceived;


void terminateHandler()
{
    try
    {
        std::ostringstream ss;
        ss << boost::stacktrace::stacktrace();

        if (g_log) 
        {
            ErLogFatal(g_log, "std::terminate() called from\n%s", ss.str().c_str());
            g_log->flush();
        }
        else
        {
            std::osyncstream(std::cerr) << "std::terminate() called from\n" << ss.str() << std::endl; // force flush
        }
    }
    catch (...)
    {
    }

    std::abort();
}

void signalHandler(int signo)
{
    g_signalReceived = signo;
    g_exitCondition.setAndNotifyAll(true);
}

void printAssertFn(std::string_view message)
{
    g_log->write(Er::Log::Level::Fatal, std::string(message));
    g_log->flush();
}


} // namespace {}


int main(int argc, char* argv[], char* env[])
{
#if ER_WINDOWS
    ::SetConsoleOutputCP(CP_UTF8);
#endif

    // set current dir the same as exe dir
    {
        std::filesystem::path exe(Er::System::CurrentProcess::exe());
        auto dir = exe.parent_path();
        std::error_code ec;
        std::filesystem::current_path(dir, ec);
    }

    // parse command line
    std::string cfgFile;

    namespace po = boost::program_options;
    
    po::variables_map vm;
    try
    {
        po::options_description cmdOpts("Command line options");
        cmdOpts.add_options()
            ("help,?", "display this message")
            ("config", po::value<std::string>(&cfgFile)->default_value("erebus-server.cfg"), "configuration file path")
#if ER_POSIX
            ("daemon,d", "run as a daemon")
            ("noroot", "don't require root privileges")
#endif
            ;

        
        po::store(po::parse_command_line(argc, argv, cmdOpts), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << cmdOpts << std::endl;
            return EXIT_SUCCESS;
        }

#if ER_POSIX
        if (!vm.contains("noroot") && (::geteuid() != 0))
        {
            std::cerr << "Root privileges required\n";
            return EXIT_FAILURE;
        }
#endif
        
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to parse the command line: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    // parse config file
    Er::Private::ServerConfig cfg;
    try
    {
        cfg = Er::Private::loadConfig(cfgFile);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to load the configuration: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
        
    if (cfg.endpoints.empty())
    {
        std::cerr << "No server endpoints specified.\n";
        return EXIT_FAILURE;
    }

    // aren't we going to be a daemon?
#if ER_POSIX
    if (vm.count("daemon"))
        Er::System::CurrentProcess::daemonize();
#endif

    std::unique_ptr<Er::Util::PidFile> pidFile;
    if (!cfg.pidfile.empty())
    {
        try
        {
            pidFile.reset(new Er::Util::PidFile(cfg.pidfile));
        }
        catch (std::exception& e)
        {
            auto existing = Er::Util::PidFile::read(cfg.pidfile);
            std::cerr << e.what() << std::endl;
            if (existing)
                std::cerr << "Found running server instance with PID " << *existing << std::endl;

            return EXIT_FAILURE;
        }
    }
    
    // setup signal handler
#if ER_POSIX
    Er::Util::SignalHandler sh({SIGINT, SIGTERM, SIGPIPE, SIGHUP});
    std::future<int> futureSigHandler =
        // spawn a thread that handles signals
        sh.asyncWaitHandler(
            [](int signo)
            {
                signalHandler(signo);
                return true;
            }
        );
#else
    ::signal(SIGINT, signalHandler);
    ::signal(SIGTERM, signalHandler);
#endif

    // setup std::terminate() handler
    std::set_terminate(terminateHandler);


    Er::Log::Level logLevel = (cfg.verbose > 0) ? Er::Log::Level::Debug : Er::Log::Level::Info;
    std::unique_ptr<Er::Private::Logger> logger;
    try
    {
        logger.reset(new Er::Private::Logger(logLevel, cfg.logfile.c_str(), cfg.keeplogs));
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    g_log = logger.get();
    Er::setPrintFailedAssertionFn(printAssertFn);

    Er::LibScope er(g_log);

    logger->unmute();

    try
    {
        auto user = Er::System::User::current();
        logger->writef(Er::Log::Level::Info, "Starting as user %s", user.name.c_str());

        std::string root;
        std::string certificate;
        std::string key;

        if (!cfg.rootCA.empty())
            root = Er::Util::loadTextFile(cfg.rootCA);

        if (!cfg.certificate.empty())
            certificate = Er::Util::loadTextFile(cfg.certificate);

        if (!cfg.privateKey.empty())
            key = Er::Util::loadTextFile(cfg.privateKey);
    
        Er::Server::LibParams srvLibParams(g_log, g_log->level());
        Er::Server::LibScope ss(srvLibParams);

        // create endpoints
        std::vector<Er::Server::IServer::Ptr> servers;
        servers.reserve(cfg.endpoints.size());
        for (auto& ep: cfg.endpoints)
        {
            logger->writef(Er::Log::Level::Info, "Creating a server instance at [%s]", ep.endpoint.c_str());

            Er::protectedCall<void>(g_log, [&ep, &root, &certificate, &key, &servers]()
            {
                Er::Server::Params params(ep.endpoint, g_log, ep.ssl, root, certificate, key);
                auto server = Er::Server::create(&params);
                servers.push_back(std::move(server));
            });
        }

        if (servers.empty())
            ErThrow("Could not create any server instances");

        Er::Private::CoreService coreService(g_log);
        for (auto& srv : servers)
        {
            coreService.registerService(srv->serviceContainer());
        }

        // load plugins
        Er::Server::PluginParams pluginParams;
        pluginParams.log = g_log;
        for (auto& srv: servers)
        {
            pluginParams.containers.push_back(srv->serviceContainer());
        }

        Er::Private::PluginMgr pluginMgr(pluginParams);
        for (auto& plugin: cfg.plugins)
        {
            if (!plugin.enabled)
            {
                logger->writef(Er::Log::Level::Info, "Skipping plugin [%s]", plugin.path.c_str());
                continue;
            }
            
            auto success = Er::protectedCall<bool>(g_log, [&plugin, &pluginMgr]()
            {
                pluginMgr.load(plugin.path, plugin.args);
                return true;
            });

            if (!success)
                logger->writef(Er::Log::Level::Error, "Failed to load plugin [%s]", plugin.path.c_str());
        }

        // now just sit around and wait
        logger->write(Er::Log::Level::Info, "Waiting for client connections...");

        g_exitCondition.waitValue(true);

        // cleanup
        logger->write(Er::Log::Level::Info, "Stopping server instances...");
        
        for (auto& srv : servers)
        {
            coreService.unregisterService(srv->serviceContainer());
        }

        pluginMgr.unloadAll();
        servers.clear();
        
        if (g_signalReceived)
        {
            logger->writef(Er::Log::Level::Warning, "Exiting due to signal %d", *g_signalReceived);
        }

        Er::setPrintFailedAssertionFn(nullptr);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
