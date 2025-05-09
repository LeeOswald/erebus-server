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
#include <erebus/rtl/logger/simple_filter.hxx>
#include <erebus/rtl/logger/simple_formatter.hxx>

#if ER_ENABLE_STACKTRACE
    #include <boost/stacktrace.hpp>
#endif

#include <clocale>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#if ER_WINDOWS
    #include <WerApi.h>
#endif


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
#if ER_ENABLE_STACKTRACE
#if ER_DEBUG
    static const std::size_t StackFramesToSkip = 4;
#else
    static const std::size_t StackFramesToSkip = 3;
#endif
    static const std::size_t StackFramesToCapture = 256;
    std::ostringstream ss;
    ss << boost::stacktrace::stacktrace{ StackFramesToSkip, StackFramesToCapture };

    Log::fatal(Er::Log::get(), "std::terminate() called from\n{}", ss.str());
#else
    Log::fatal(Er::Log::get(), "std::terminate() called");
#endif
    Er::Log::get()->flush(std::chrono::milliseconds(5000));

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
    Er::Log::get()->flush(std::chrono::milliseconds(5000));
}

bool Program::setLocale(const char* locale)
{
    std::string oldLocale;
    {
        auto tmp = std::setlocale(LC_ALL, nullptr);
        if (!tmp)
            return false;

        const auto length = ::strnlen(tmp, 1024);
        if (length == 1024)
            return false;

        oldLocale.assign(tmp, length);
    }

    static const char* const CLocale = "C";
    
    const char* newLocale = (!locale || !*locale) ? CLocale : locale;

    if (!std::setlocale(LC_ALL, newLocale))
        return false;

#if ER_POSIX
    // provide child processes with an actual locale via the "LC_ALL" environment.
    if (::setenv("LC_ALL", newLocale, 1) == -1)
    {
        std::setlocale(LC_ALL, oldLocale.c_str());
        return false;
    }
#endif

    std::locale::global(std::locale());

    return true;
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
    ::SetConsoleCP(CP_UTF8);
    ::SetConsoleOutputCP(CP_UTF8);

    ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    ::WerSetFlags(WER_FAULT_REPORTING_NO_UI);
#endif

    std::string locale("en_US.UTF-8");
    if (auto lang = std::getenv("LANG"))
    {
        locale = lang;
    }

    if (!setLocale(locale.c_str()))
    {
        std::cerr << "Failed to set locale " << locale << "\n";
    }

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
            Log::makeSimpleFormatter(formatOptions),
            Log::FilterPtr{}
        );

        main->addSink("debugger", sink);
    }
#endif

#if ER_LINUX
    if (m_isDaemon)
    {
        auto sink = Log::makeSyslogSink(
            "erebus", 
            Log::makeSimpleFormatter(formatOptions),
            Log::makeLevelFilter(Log::Level::Error)
        );

        main->addSink("syslog", sink);
    }
#endif

    if (!m_isDaemon)
    {
        auto sink = makeOStreamSink(
            std::cout,
            Log::makeSimpleFormatter(formatOptions),
            Log::makeLevelFilter(Log::Level::Debug, Log::Level::Info)
        );

        main->addSink("std::cout", sink);
    }

    if (!m_isDaemon)
    {
        auto sink = makeOStreamSink(
            std::cerr,
            Log::makeSimpleFormatter(formatOptions),
            Log::makeLevelFilter(Log::Level::Warning)
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
        m_logger->addSink("tee", tee.cast<Log::ISink>());

        addLoggers(tee.get());
    }
    else
    {
        auto tee = Log::makeTee(ThreadSafe::No); // tee is called from the single logger thread
        m_logger->addSink("tee", tee.cast<Log::ISink>());

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
        run(argc, argv);
    }
    catch(...)
    {
        dispatchException(std::current_exception(), xcptLogger);
    }

    Er::Log::get()->flush(std::chrono::milliseconds(5000));

    globalShutdown();

    return resut;
}


} // namespace Er {}