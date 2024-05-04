#include <boost/process.hpp>
#include <boost/program_options.hpp>
#include <boost/stacktrace.hpp>

#include <erebus/condition.hxx>
#include <erebus/knownprops.hxx>
#include <erebus/syncstream.hxx>
#include <erebus/system/process.hxx>
#include <erebus/system/thread.hxx>
#include <erebus/system/user.hxx>
#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/file.hxx>
#include <erebus/util/format.hxx>
#include <erebus/util/sha256.hxx>
#if ER_POSIX
    #include <erebus/util/signalhandler.hxx>
#endif

#include <erebus-srv/erebus-srv.hxx>

#include "config.hxx"
#include "coreservice.hxx"
#include "logger.hxx"
#include "pluginmgr.hxx"
#include "users.hxx"

#include <filesystem>
#include <future>
#include <iostream>
#include <vector>


namespace
{

Er::Log::ILog* g_log = nullptr;
Er::Event g_exitCondition(false);
std::optional<int> g_signalReceived;


void terminateHandler()
{
    std::ostringstream ss;
    ss << boost::stacktrace::stacktrace();

    if (g_log)
        ErLogFatal(g_log, ErLogNowhere(), "std::terminate() called from\n%s", ss.str().c_str());
    else
        Er::osyncstream(std::cerr) << "std::terminate() called from\n" << ss.str();

    std::abort();
}

void signalHandler(int signo)
{
    g_signalReceived = signo;
    g_exitCondition.setAndNotifyAll(true);
}


} // namespace {}


int main(int argc, char* argv[], char* env[])
{
#if ER_WINDOWS
    ::SetConsoleOutputCP(CP_UTF8);
#endif

#if ER_POSIX
    if (::geteuid() != 0)
    {
        std::cerr << "Root privileges required\n";
        return EXIT_FAILURE;
    }
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
#endif
            ;

        
        po::store(po::parse_command_line(argc, argv, cmdOpts), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << cmdOpts << "\n";
            return EXIT_SUCCESS;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to parse the command line: " << e.what() << "\n";
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
        std::cerr << "Failed to load the configuration: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
        
    if (cfg.userDb.empty())
    {
        std::cerr << "User DB path expected\n";
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
    auto logger = std::make_unique<Er::Private::Logger>(logLevel, cfg.logfile.c_str());
    if (!logger->valid())
        return EXIT_FAILURE;

    g_log = logger.get();

    Er::LibScope er(g_log);

    logger->unmute();

    try
    {
        auto user = Er::System::User::current();
        logger->write(Er::Log::Level::Info, ErLogNowhere(), "Starting as user %s", user.name.c_str());

        std::string root;
        std::string certificate;
        std::string key;

        if (!cfg.rootCA.empty())
            root = Er::Util::loadTextFile(cfg.rootCA);

        if (!cfg.certificate.empty())
            certificate = Er::Util::loadTextFile(cfg.certificate);

        if (!cfg.privateKey.empty())
            key = Er::Util::loadTextFile(cfg.privateKey);
    
        Er::Private::UserDb userDb(cfg.userDb);

        Er::Server::Private::LibParams srvLibParams(g_log, g_log->level());
        Er::Server::Private::LibScope ss(srvLibParams);

        // create endpoints
        std::vector<std::shared_ptr<Er::Server::Private::IServer>> servers;
        servers.reserve(cfg.endpoints.size());
        for (auto& ep: cfg.endpoints)
        {
            logger->write(Er::Log::Level::Info, ErLogNowhere(), "Creating a server instance at [%s]", ep.endpoint.c_str());

            try
            {
                Er::Server::Private::Params params(ep.endpoint, g_log, ep.ssl, root, certificate, key, &userDb);
                auto server = Er::Server::Private::create(&params);
                servers.push_back(server);
            }
            catch (Er::Exception& e)
            {
                Er::Util::logException(g_log, Er::Log::Level::Error, e);
            }
            catch (std::exception& e)
            {
                Er::Util::logException(g_log, Er::Log::Level::Error, e);
            }
        }

        if (servers.empty())
            throw Er::Exception(ER_HERE(), "Could not create any server instances");

        Er::Private::CoreService coreService(g_log, &userDb);
        for (auto srv : servers)
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
                logger->write(Er::Log::Level::Info, ErLogNowhere(), "Skipping plugin [%s]", plugin.path.c_str());
                continue;
            }
            
            try
            {
                pluginMgr.load(plugin.path, plugin.args);
            }
            catch (Er::Exception& e)
            {
                Er::Util::logException(g_log, Er::Log::Level::Error, e);
            }
            catch (std::exception& e)
            {
                Er::Util::logException(g_log, Er::Log::Level::Error, e);
            }
        }

        // now just sit around and wait
        logger->write(Er::Log::Level::Info, ErLogNowhere(), "Waiting for client connections...");

        g_exitCondition.waitValue(true);

        // cleanup
        logger->write(Er::Log::Level::Info, ErLogNowhere(), "Stopping server instances...");
        
        for (auto srv : servers)
        {
            coreService.unregisterService(srv->serviceContainer());
        }

        pluginMgr.unloadAll();
        servers.clear();
        
        if (g_signalReceived)
        {
            logger->write(Er::Log::Level::Warning, ErLogNowhere(), "Exiting due to signal %d", *g_signalReceived);
        }
    }
    catch (Er::Exception& e)
    {
        Er::Util::logException(g_log, Er::Log::Level::Fatal, e);
        return EXIT_FAILURE;
    }
    catch (std::exception& e)
    {
        Er::Util::logException(g_log, Er::Log::Level::Fatal, e);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
