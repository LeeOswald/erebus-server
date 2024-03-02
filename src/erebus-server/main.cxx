#include <boost/process.hpp>
#include <boost/program_options.hpp>
#include <boost/stacktrace.hpp>

#include <erebus/condition.hxx>
#include <erebus/knownprops.hxx>
#include <erebus/system/process.hxx>
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
#include "logger.hxx"
#include "pluginmgr.hxx"
#include "users.hxx"

#include <future>
#include <iostream>
#include <vector>


namespace
{

Er::Log::ILog* g_log = nullptr;
Er::Event g_exitCondition(false);
bool g_restartRequired = false;
std::optional<int> g_signalReceived;


void terminateHandler()
{
    std::ostringstream ss;
    ss << boost::stacktrace::stacktrace();

    if (g_log)
        LogFatal(g_log, LogNowhere(), "std::terminate() called from\n%s", ss.str().c_str());
    else
        std::cerr << "std::terminate() called from\n" << ss.str();

    std::abort();
}

void signalHandler(int signo)
{
    g_signalReceived = signo;
    g_exitCondition.setAndNotifyAll(true);
}

void restart(int argc, char* argv[], char* env[])
{
    std::vector<std::string> args;
    args.reserve(argc);
    args.push_back(Er::System::CurrentProcess::exe());
    for (int i = 1; i < argc; ++i)
    {
        args.push_back(std::string(argv[i]));
    }

    boost::process::spawn(std::move(args));
}

} // namespace {}


int main(int argc, char* argv[], char* env[])
{
#if ER_WINDOWS
    ::SetConsoleOutputCP(CP_UTF8);
#endif

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

    Er::LibScope er;

    Er::Log::Level logLevel = (cfg.verbose > 0) ? Er::Log::Level::Debug : Er::Log::Level::Info;
    auto logger = std::make_unique<Er::Private::Logger>(logLevel, cfg.logfile.c_str());
    if (!logger->valid())
        return EXIT_FAILURE;

    g_log = logger.get();
    logger->unmute();

    try
    {
        auto user = Er::System::User::current();
        logger->write(Er::Log::Level::Info, LogNowhere(), "Starting as user %s", user.name.c_str());

        std::string root;
        std::string certificate;
        std::string key;

        if (!cfg.rootCA.empty())
            root = Er::Util::loadFile(cfg.rootCA);

        if (!cfg.certificate.empty())
            certificate = Er::Util::loadFile(cfg.certificate);

        if (!cfg.privateKey.empty())
            key = Er::Util::loadFile(cfg.privateKey);
    
        Er::Private::UserDb userDb(cfg.userDb);

        Er::Server::Private::LibParams srvLibParams(g_log, g_log->level());
        Er::Server::Private::LibScope ss(srvLibParams);

        // create endpoints
        std::vector<std::shared_ptr<Er::Server::Private::IServer>> servers;
        servers.reserve(cfg.endpoints.size());
        for (auto& ep: cfg.endpoints)
        {
            logger->write(Er::Log::Level::Info, LogNowhere(), "Creating a server instance at [%s]", ep.endpoint.c_str());

            try
            {
                Er::Server::Private::Params params(ep.endpoint, g_log, &g_exitCondition, &g_restartRequired, ep.ssl, root, certificate, key, &userDb);
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

        // load plugins
        Er::Server::PluginParams pluginParams;
        pluginParams.log = g_log;
        for (auto& srv: servers)
        {
            pluginParams.containers.push_back(srv->serviceContainer());
        }

        Er::Private::PluginMgr pluginMgr(pluginParams);
        for (auto& path: cfg.plugins)
        {
            try
            {
                pluginMgr.load(path);
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
        logger->write(Er::Log::Level::Info, LogNowhere(), "Waiting for client connections...");

        g_exitCondition.waitValue(true);

        // cleanup
        logger->write(Er::Log::Level::Info, LogNowhere(), "Stopping server instances...");
        
        // we must explicitly stop the server to make the listening endpoint addresses available again
        // before we spawn a copy of us during the restart command 
        pluginMgr.unloadAll();
        servers.clear();
        
        if (g_signalReceived)
        {
            logger->write(Er::Log::Level::Warning, LogNowhere(), "Exiting due to signal %d", *g_signalReceived);
        }
        else if (!g_restartRequired)
        {
            logger->write(Er::Log::Level::Warning, LogNowhere(), "Shutting down...");
        }
        else
        {
            logger->write(Er::Log::Level::Warning, LogNowhere(), "Restarting...");
            g_log = nullptr;
            // force logger destruction to unlock the logfile
            logger.reset();
            restart(argc, argv, env);
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
