#include <boost/process.hpp>
#include <boost/program_options.hpp>
#include <boost/stacktrace.hpp>

#include <erebus/knownprops.hxx>
#include <erebus/system/process.hxx>
#include <erebus/util/condition.hxx>
#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/format.hxx>
#include <erebus-srv/erebus-srv.hxx>


#include "logger.hxx"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>


#if ER_POSIX
    #include <fcntl.h>
    #include <sys/stat.h>
#endif


namespace
{

Er::Log::ILog* g_log = nullptr;
Er::Util::Condition g_exitCondition(Er::Util::Condition::Reset::Manual);
bool g_restartRequired = false;
std::optional<int> g_signalReceived;

std::string loadFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        throw Er::Exception(ER_HERE(), Er::Util::format("Failed to open [%s]", path.c_str()));

    std::stringstream ss;
    ss << file.rdbuf();

    return ss.str();
}

void terminateHandler()
{
    std::ostringstream ss;
    ss << boost::stacktrace::stacktrace();

    if (g_log)
        LogFatal(g_log, "std::terminate() called from\n%s", ss.str().c_str());
    else
        std::cerr << "std::terminate() called from\n" << ss.str();

    std::abort();
}

void signalHandler(int signo)
{
    g_signalReceived = signo;
    g_exitCondition.set();
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

#if ER_LINUX && !ER_DEBUG
    if (::geteuid() != 0)
    {
        std::cerr << "Root privileges required\n";
        return EXIT_FAILURE;
    }
#endif

    std::string logFile;
    std::string cfgFile;
    std::vector<std::string> endpoints;
    std::string rootFile;
    std::string certFile;
    std::string keyFile;

    namespace po = boost::program_options;
    po::options_description options("Command line options");
    options.add_options()
        ("help,h", "display this message")
        ("verbose,v", "display debug output")
        ("logfile,l", po::value<std::string>(&logFile)->default_value("erebus-server.log"), "log file path")
        ("config,c", po::value<std::string>(&cfgFile)->default_value("erebus-server.cfg"), "configuration file path")
#if ER_POSIX
        ("daemon,d", "run as a daemon")
#endif
        ("endpoint,e", po::value<decltype(endpoints)>(&endpoints), "server endpoint")
        ("root,r", po::value<std::string>(&rootFile), "root certificate file path")
        ("certificate,s", po::value<std::string>(&certFile), "certificate file path")
        ("key,k", po::value<std::string>(&keyFile), "private key file path")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, options), vm);
    po::notify(vm);

    std::ifstream cfg(cfgFile, std::ifstream::in);
    if (cfg.good())
    {
        po::store(po::parse_config_file(cfg, options), vm);
        cfg.close();
        po::notify(vm);        
    }
    

    if (vm.count("help"))
    {
        std::cerr << options << "\n";
        return EXIT_SUCCESS;
    }

    if (endpoints.empty())
    {
        std::cerr << "No server endpoints specified.\n";
        return EXIT_FAILURE;
    }

#if ER_POSIX
    if (vm.count("daemon"))
        Er::System::CurrentProcess::daemonize();
#endif

    Er::Scope er;

    Er::Log::Level logLevel = vm.count("verbose") ? Er::Log::Level::Debug : Er::Log::Level::Info;
    auto logger = std::make_unique<Er::Private::Logger>(logLevel, logFile.c_str());

    if (!logger->exclusive())
        return EXIT_FAILURE;

    g_log = logger.get();
    logger->unmute();

    std::set_terminate(terminateHandler);

    try
    {
        
        ::signal(SIGINT, signalHandler);
        ::signal(SIGTERM, signalHandler);
#if ER_POSIX
        ::signal(SIGPIPE, signalHandler);
        ::signal(SIGHUP, signalHandler);
#endif

        std::string root;
        std::string certificate;
        std::string key;

        if (!rootFile.empty())
            root = loadFile(rootFile);

        if (!certFile.empty())
            certificate = loadFile(certFile);

        if (!keyFile.empty())
            key = loadFile(keyFile);

        Er::Private::Server::Scope ss;

        std::vector<std::shared_ptr<Er::Private::Server::IServer>> servers;
        servers.reserve(endpoints.size());
        for (auto ep: endpoints)
        {
            logger->write(Er::Log::Level::Info, "Creating a server instance at [%s]", ep.c_str());

            try
            {
                Er::Private::Server::Params params(ep, g_log, &g_exitCondition, &g_restartRequired, root, certificate, key);
                auto server = Er::Private::Server::start(&params);
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

        logger->write(Er::Log::Level::Info, "Waiting for client connections...");

        g_exitCondition.wait();

        for (auto srv: servers)
        {
            srv->stop();
        }
        
        if (g_signalReceived)
        {
            logger->write(Er::Log::Level::Warning, "Exiting due to signal %d", *g_signalReceived);
        }
        else if (!g_restartRequired)
        {
            logger->write(Er::Log::Level::Warning, "Shutting down...");
        }
        else
        {
            logger->write(Er::Log::Level::Warning, "Restarting...");
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
