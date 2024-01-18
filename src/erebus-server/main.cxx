#include <erebus/knownprops.hxx>
#include <erebus/util/condition.hxx>
#include <erebus/util/exceptionutil.hxx>

#include "logger.hxx"

#include <iostream>
#include <sstream>

#include <boost/program_options.hpp>
#include <boost/stacktrace.hpp>

#if ER_POSIX
    #include <fcntl.h>
    #include <sys/stat.h>
#endif

namespace
{

Er::Log::ILog* g_log = nullptr;
Er::Util::Condition g_exitCondition(false);
std::optional<int> g_signalReceived;


#if ER_POSIX
void daemonize() noexcept
{
    // Fork the process and have the parent exit. If the process was started
    // from a shell, this returns control to the user. Forking a new process is
    // also a prerequisite for the subsequent call to setsid().
    auto pid = ::fork();

    if (pid < 0)
        ::exit(EXIT_FAILURE);

    if (pid > 0)
        ::exit(EXIT_SUCCESS);

    // Make the process a new session leader. This detaches it from the terminal.
    if (::setsid() < 0)
        ::exit(EXIT_FAILURE);

    // Close the standard streams. This decouples the daemon from the terminal that started it.
    auto null = ::open("/dev/null", O_RDWR);
    ::dup2(null, STDIN_FILENO);
    ::dup2(null, STDOUT_FILENO);
    ::dup2(null, STDERR_FILENO);
    ::close(null);

    // A process inherits its working directory from its parent. This could be
    // on a mounted filesystem, which means that the running daemon would
    // prevent this filesystem from being unmounted. Changing to the root
    // directory avoids this problem.
    ::chdir("/");

    ::signal(SIGCHLD, SIG_IGN);
    ::signal(SIGHUP, SIG_IGN);

    // A second fork ensures the process cannot acquire a controlling terminal.
    pid = ::fork();

    if (pid < 0)
        ::exit(EXIT_FAILURE);

    if (pid > 0)
        ::exit(EXIT_SUCCESS);

    // The file mode creation mask is also inherited from the parent process.
    // We don't want to restrict the permissions on files created by the
    // daemon, so the mask is cleared.
    ::umask(0);
}
#endif

void terminateHandler()
{
    std::ostringstream ss;
    ss << boost::stacktrace::stacktrace();

    LogFatal(g_log, "std::terminate() called from\n%s", ss.str().c_str());

    std::abort();
}

void signalHandler(int signo)
{
    g_signalReceived = signo;
    g_exitCondition.set();
}

} // namespace {}


int main(int argc, char* argv[])
{
#if ER_LINUX && !ER_DEBUG
    if (::geteuid() != 0)
    {
        std::cerr << "Root privileges required\n";
        return EXIT_FAILURE;
    }
#endif


    namespace po = boost::program_options;
    po::options_description options("Command line options");
    options.add_options()
        ("help,h", "display this message")
        ("verbose,v", "display debug output")
#if ER_POSIX
        ("daemon,d", "run as a daemon")
#endif
        ("address,a", po::value<std::string>(), "server bind address:port")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, options), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        std::cerr << options << "\n";
        return EXIT_SUCCESS;
    }

#if ER_POSIX
    if (vm.count("daemon"))
        daemonize();
#endif

    Er::initialize();

    Er::Log::Level logLevel = vm.count("verbose") ? Er::Log::Level::Debug : Er::Log::Level::Info;

#if ER_LINUX && !ER_DEBUG
    Er::Private::Logger logger(logLevel, "/var/log/erebus-server.log");
#else
    Er::Private::Logger logger(logLevel, "erebus-server.log");
#endif

    if (!logger.exclusive())
        return EXIT_FAILURE;

    g_log = &logger;

    std::set_terminate(terminateHandler);

    try
    {
        std::string bindAddr("127.0.0.1:6665");
        if (vm.count("address"))
        {
            bindAddr = vm["address"].as<std::string>();
        }

        logger.write(Er::Log::Level::Info, "Binding to address %s", bindAddr.c_str());

        ::signal(SIGINT, signalHandler);
        ::signal(SIGTERM, signalHandler);
#if ER_POSIX
        ::signal(SIGPIPE, signalHandler);
        ::signal(SIGHUP, signalHandler);
#endif

        g_exitCondition.wait();
        if (g_signalReceived)
        {
            logger.write(Er::Log::Level::Warning, "Exiting due to signal %d", *g_signalReceived);
        }

        Er::finalize();

        g_log = nullptr;
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
