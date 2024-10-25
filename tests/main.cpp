#include "common.hpp"

#include <erebus/log.hxx>
#if ER_WINDOWS
    #include <erebus/util/utf16.hxx>
#endif

#include <iostream>
#include <sstream>

#include <boost/stacktrace.hpp>

#if ER_DEBUG && defined(_MSC_VER)
#include <crtdbg.h>
#endif

Er::Log::ILog* g_log = nullptr;

int InstanceCounter::instances = 0;


void terminateHandler()
{
    std::ostringstream ss;
    ss << boost::stacktrace::stacktrace();

    std::osyncstream(std::cerr) << "std::terminate() called from\n" << ss.str();

    std::abort();
}

int main(int argc, char** argv)
{
#if ER_POSIX
    // globally block signals so that child threads, e.g., logger 
    // inherit signal mask with signals blocked 
    sigset_t mask;
    ::sigemptyset(&mask);
    ::sigaddset(&mask, SIGTERM);
    ::sigaddset(&mask, SIGHUP);
    ::sigaddset(&mask, SIGINT);
    ::sigaddset(&mask, SIGUSR1);
    ::sigaddset(&mask, SIGUSR2);
    ::sigprocmask(SIG_BLOCK, &mask, nullptr);
#endif

    // setup std::terminate() handler
    std::set_terminate(terminateHandler);

#if ER_DEBUG && defined(_MSC_VER)
    int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(tmpFlag);
    //_CrtSetBreakAlloc(2127);
#endif

#if ER_WINDOWS
    ::SetConsoleOutputCP(CP_UTF8);
#endif

    ::testing::InitGoogleTest(&argc, argv);

    int ret = EXIT_FAILURE;

    try
    {
        auto logger = Er::Log::makeAsyncLogger();

#if ER_WINDOWS
        if (::IsDebuggerPresent())
        {
            auto debugger = Er::Log::makeDebuggerSink(
                Er::Log::SimpleFormatter::make({ Er::Log::SimpleFormatter::Option::Time, Er::Log::SimpleFormatter::Option::Level, Er::Log::SimpleFormatter::Option::Tid })
            );

            logger->addSink("debugger", debugger);
        }
#endif

        {
            auto stdoutSink = Er::Log::makeOStreamSink(
                std::cout,
                Er::Log::SimpleFormatter::make({ }),
                Er::Log::SimpleFilter::make(Er::Log::Level::Debug, Er::Log::Level::Warning)
            );

            logger->addSink("stdout", stdoutSink);

            auto stderrSink = Er::Log::makeOStreamSink(
                std::cerr,
                Er::Log::SimpleFormatter::make({ }),
                Er::Log::SimpleFilter::make(Er::Log::Level::Error, Er::Log::Level::Fatal)
            );

            logger->addSink("stderr", stderrSink);

            logger->setLevel(Er::Log::Level::Debug);
        }

        g_log = logger.get();
        Er::LibScope er(g_log);


#if ER_POSIX
        // unblock signals in the test thread
        ::sigemptyset(&mask);
        ::sigaddset(&mask, SIGTERM);
        ::sigaddset(&mask, SIGINT);
        ::sigaddset(&mask, SIGUSR1);
        ::sigaddset(&mask, SIGUSR2);
        ::sigprocmask(SIG_UNBLOCK, &mask, nullptr);
#endif

        TestProps::registerAll(g_log);
    
        ret = RUN_ALL_TESTS();

        TestProps::unregisterAll(g_log);
        g_log = nullptr;
    }
    catch (std::exception& e)
    {
        std::osyncstream(std::cerr) << "Unexpected error: " << e.what() << "\n";
    }

    return ret;
}

