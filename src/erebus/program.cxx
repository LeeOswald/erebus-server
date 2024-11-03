#include <erebus/program.hxx>
#include <erebus/system/process.hxx>
#include <erebus/system/thread.hxx>
#include <erebus/util/exceptionutil.hxx>

#include <iostream>
#include <filesystem>

#include <boost/stacktrace.hpp>

#if ER_DEBUG && defined(_MSC_VER)
#include <crtdbg.h>
#endif

namespace Er
{

Program* Program::s_instance = nullptr;
bool Program::s_isDaemon = false;
Event Program::s_exitCondition(false);
int Program::s_signalReceived = 0;

#if ER_POSIX
std::unique_ptr<SignalWaiter> s_signalWaiter;
#endif

void Program::globalStartup(int argc, char** argv) noexcept
{
#if ER_POSIX
    bool daemonize = optionPresent(argc, argv, "--daemon", "-d");
    if (daemonize)
    {
        Er::System::CurrentProcess::daemonize();
        s_isDaemon = true;
    }
#endif

#if ER_WINDOWS
    ::SetConsoleOutputCP(CP_UTF8);
#endif

#if ER_DEBUG && defined(_MSC_VER)
    int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(tmpFlag);
#endif

#if ER_POSIX
    s_signalWaiter.reset(new SignalWaiter{ SIGINT, SIGTERM, SIGPIPE, SIGHUP });
#else
    ::signal(SIGINT, staticSignalHandler);
    ::signal(SIGTERM, staticSignalHandler);
#endif

    std::set_terminate(staticTerminateHandler);

    setPrintFailedAssertionFn(staticPrintAssertFn);

    // set current dir the same as exe dir
    {
        std::filesystem::path exe(Er::System::CurrentProcess::exe());
        auto dir = exe.parent_path();
        std::error_code ec;
        std::filesystem::current_path(dir, ec);
    }
}

void Program::globalShutdown() noexcept
{
#if ER_POSIX
    s_signalWaiter.reset();
#endif

    Er::setPrintFailedAssertionFn(nullptr);
}

Program::~Program()
{
    s_instance = nullptr;
}

Program::Program() noexcept
{
    ErAssert(!s_instance);
    s_instance = this;
}

bool Program::loadConfiguration(int argc, char** argv) noexcept
{
    try
    {
        boost::program_options::options_description options("Command line options");
        options.add_options()
            ("help,?", "show help")
            ("verbose,v", "verbose logging")
#if ER_POSIX
            ("daemon,d", "run as a daemon")
#endif
            ;

        addCmdLineOptions(options);

        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, options), m_options);
        boost::program_options::notify(m_options);

        if (m_options.count("help"))
        {
            displayHelp(options);
            return false;
        }
    
        return doLoadConfiguration();
    }
    catch (std::exception& e)
    {
        std::cerr << "Unexpected exception: " << e.what() << std::endl;
    }

    return false;
}

void Program::addCmdLineOptions(boost::program_options::options_description& options)
{
}

bool Program::doLoadConfiguration()
{
    return true;
}

void Program::displayHelp(const boost::program_options::options_description& options)
{
    std::cout << options << std::endl;
}

bool Program::verboseLogging() const noexcept
{
    return m_options.count("verbose") > 0;
}

void Program::staticTerminateHandler()
{
    std::ostringstream ss;
    ss << boost::stacktrace::stacktrace();

    Log::fatal(Log::defaultLog(), "std::terminate() called from\n{}", ss.str());

    std::abort();
}

void Program::staticSignalHandler(int signo)
{
    s_signalReceived = signo;
    s_exitCondition.setAndNotifyAll(true);
}

void Program::staticPrintAssertFn(std::string_view message)
{
    Log::writeln(Log::defaultLog(), Log::Level::Fatal, std::string(message));
}

bool Program::initialize(int argc, char** argv) noexcept
{
    if (!loadConfiguration(argc, argv))
        return false;

    if (!initializeRtl())
        return false;

    try
    {
        System::CurrentThread::setName("main");

        doInitialize();

        return true;
    }
    catch (Exception& e)
    {
        Util::logException(Log::defaultLog(), Log::Level::Fatal, e);
    }
    catch (std::exception& e)
    {
        Util::logException(Log::defaultLog(), Log::Level::Fatal, e);
    }

    // failure
    finalizeRtl();
    return false;
}

void Program::finalize() noexcept
{
    doFinalize();
    finalizeRtl();
}

bool Program::initializeRtl() noexcept
{
    try
    {
        m_logger = Log::makeAsyncLogger();
        addLoggers(m_logger.get());

        if (verboseLogging())
            m_logger->setLevel(Er::Log::Level::Debug);
        else
            m_logger->setLevel(Er::Log::Level::Info);

        Er::initialize(m_logger.get());

        return true;
    }
    catch (std::exception& e)
    {
        Er::Log::fatal(Log::defaultLog(), "Unexpected exception {}", e.what());
    }

    return false;
}

void Program::finalizeRtl() noexcept
{
    Er::finalize(m_logger.get());
    m_logger->flush();
    m_logger.reset();
}

int Program::run(int argc, char** argv) noexcept
{
    int result = EXIT_FAILURE;
    
    if (!initialize(argc, argv))
        return result;
        
    try
    {
        result = doRun(argc, argv);
    }
    catch (Exception& e)
    {
        Util::logException(m_logger.get(), Log::Level::Fatal, e);
    }
    catch (std::exception& e)
    {
        Util::logException(m_logger.get(), Log::Level::Fatal, e);
    }

    finalize();

    return result;
}

} // namespace Er {}