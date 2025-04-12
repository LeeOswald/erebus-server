#include <erebus/rtl/program.hxx>
#include <erebus/rtl/system/process.hxx>
#include <erebus/rtl/util/exception_util.hxx>

#if ER_WINDOWS
    #include <erebus/rtl/logger/win32_debugger_sink.hxx>
#endif

#if ER_LINUX
    #include <erebus/rtl/logger/syslog_linux_sink.hxx>
#endif

#include <erebus/rtl/logger/ostream_sink.hxx>
#include <erebus/rtl/logger/simple_formatter.hxx>
#include <erebus/rtl/type_id.hxx>

#include <boost/stacktrace.hpp>

#include <csignal>
#include <filesystem>
#include <iostream>



namespace Er
{

Program* Program::s_instance = nullptr;


Program::~Program()
{
    Erp::Log::setGlobal({});
    s_instance = nullptr;
}

Program::Program(int options) noexcept
    : m_options(options)
{
    // no complex initialization here
    s_instance = this;
}

bool Program::argPresent(int argc, char** argv, const char* longName, const char* shortName) noexcept
{
    for (int i = 1; i < argc; ++i)
    {
        if (longName && !std::strcmp(argv[i], longName))
            return true;

        if (shortName && !std::strcmp(argv[i], shortName))
            return true;
    }

    return false;
}

void Program::staticTerminateHandler()
{
    if (s_instance)
        s_instance->terminateHandler();
    else
        std::abort();
}

void Program::terminateHandler()
{
    std::ostringstream ss;
    ss << boost::stacktrace::stacktrace();

    Log::fatal(Er::Log::get(), "std::terminate() called from\n{}", ss.str());
    Er::Log::get()->flush();

    std::abort();
}

void Program::signalHandler(int signo)
{
    m_signalReceived = signo;
    m_exitCondition.setAndNotifyAll(true);
}

void Program::staticPrintAssertFn(std::string_view message)
{
    if (s_instance)
        s_instance->printAssertFn(message);
}

void Program::printAssertFn(std::string_view message)
{
    Log::writeln(Er::Log::get(), Log::Level::Fatal, std::string(message));
    Er::Log::get()->flush();
}

void Program::globalStartup(int argc, char** argv) noexcept
{
#if ER_POSIX
    // daemonize as early as possible to avoid a whole bunch of bugs
    if (m_options & CanBeDaemonized)
    {
        bool daemonize = argPresent(argc, argv, "--daemon", "-d");
        if (daemonize)
        {
            Er::System::CurrentProcess::daemonize();
            m_isDaemon = true;
        }
    }
#endif

#if ER_WINDOWS
    ::SetConsoleOutputCP(CP_UTF8);
#endif

#if ER_DEBUG && defined(_MSC_VER)
    // use MSVC debug heap
    int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(tmpFlag);
#endif

    // setup signal handler
    if (m_options & EnableSignalHandler)
    {
#if ER_WINDOWS
        m_signalWaiter.reset(new SignalWaiter(this, { SIGINT, SIGTERM }));
#else
        m_signalWaiter.reset(new SignalWaiter(this, { SIGINT, SIGTERM, SIGPIPE, SIGHUP }));
#endif
    }
    
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
    if (m_signalWaiter)
    {
#if ER_POSIX
        // m_signalWaiter is locked in sigwait() so wake it 
        ::kill(::getpid(), SIGHUP);
#endif
        m_signalWaiter.reset();
    }

    Er::setPrintFailedAssertionFn(nullptr);
}


void Program::addCmdLineOptions(boost::program_options::options_description& options)
{
}

void Program::displayHelp(const boost::program_options::options_description& options)
{
    std::cout << options << std::endl;
}

bool Program::loadConfiguration()
{
    return true;
}

void Program::addLoggers(Log::ITee* main)
{
    auto formatOptions = Er::Log::SimpleFormatter::Options{
        Er::Log::SimpleFormatter::Option::Option::Time,
        Er::Log::SimpleFormatter::Option::Option::Level,
        Er::Log::SimpleFormatter::Option::Option::Tid,
        Er::Log::SimpleFormatter::Option::Option::TzLocal,
        Er::Log::SimpleFormatter::Option::Option::Lf,
        Er::Log::SimpleFormatter::Option::Option::Component
    };

#if ER_WINDOWS
    if (isDebuggerPresent())
    {
        auto sink = Log::makeDebuggerSink(
            Log::SimpleFormatter::make(formatOptions),
            Log::Filter{}
        );

        main->addSink("debugger", sink);
    }
#endif

#if ER_LINUX
    if (m_isDaemon)
    {
        auto sink = Log::makeSyslogSink(
            "erebus", 
            Log::SimpleFormatter::make(formatOptions),
            [](const Log::Record* r) { return r->level() >= Log::Level::Error; }
        );

        main->addSink("syslog", sink);
    }
#endif

    if (!m_isDaemon)
    {
        auto sink = makeOStreamSink(
            std::cout,
            Log::SimpleFormatter::make(formatOptions),
            [](const Log::Record* r)
            {
                return r->level() < Er::Log::Level::Error;
            }
        );

        main->addSink("std::cout", sink);
    }

    if (!m_isDaemon)
    {
        auto sink = makeOStreamSink(
            std::cerr,
            Log::SimpleFormatter::make(formatOptions),
            [](const Log::Record* r)
            {
                return r->level() >= Er::Log::Level::Error;
            }
        );

        main->addSink("std::cerr", sink);
    }
}

bool Program::globalLoadConfiguration(int argc, char** argv)
{
    boost::program_options::options_description options("Command line options");
    
    try
    {
        options.add_options()
            ("help,?", "show help")
            ("verbose,v", "verbose logging")
            ("logthreshold", boost::program_options::value<unsigned>(&m_loggerThreshold)->default_value(unsigned(1000)), "lt");


#if ER_POSIX
        if (m_options & CanBeDaemonized)
            options.add_options()
            ("daemon,d", "run as a daemon");
#endif

        addCmdLineOptions(options);

        boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(options).allow_unregistered().run(), m_args);
        boost::program_options::notify(m_args);
    }
    catch (boost::program_options::error& e)
    {
        std::cerr << e.what() << std::endl;
        displayHelp(options);
        return false;
    }

    if (m_args.count("help"))
    {
        displayHelp(options);
        return false;
    }

    return loadConfiguration();
}

void Program::globalMakeLogger()
{
    if (m_options & SyncLogger)
        m_logger = Log::makeSyncLogger();
    else
        m_logger = Log::makeLogger({}, std::chrono::milliseconds(m_loggerThreshold));

    auto verbose = m_args.count("verbose") > 0;
    m_logger->setLevel(verbose ? Log::Level::Debug : Log::Level::Info);

    Er::Log::g_verbose = verbose;

    if (m_options & SyncLogger)
    {
        auto tee = Log::makeTee(ThreadSafe::Yes);
        m_logger->addSink("tee", std::static_pointer_cast<Log::ISink>(tee));

        addLoggers(tee.get());
    }
    else
    {
        auto tee = Log::makeTee(ThreadSafe::No); // tee is called from the single logger thread
        m_logger->addSink("tee", std::static_pointer_cast<Log::ISink>(tee));

        addLoggers(tee.get());
    }

    Erp::Log::setGlobal(m_logger);
}

int Program::exec(int argc, char** argv)
{
    int resut = EXIT_FAILURE;
    globalStartup(argc, argv);
    
    if (!globalLoadConfiguration(argc, argv))
        return EXIT_FAILURE;

    globalMakeLogger();
    
    Util::ExceptionLogger xcptLogger(m_logger.get());

    try
    {
        initializeTypeRegistry(m_logger.get());
        run(argc, argv);
        finalizeTypeRegistry();
    }
    catch(...)
    {
        dispatchException(std::current_exception(), xcptLogger);
    }

    globalShutdown();

    return resut;
}


} // namespace Er {}