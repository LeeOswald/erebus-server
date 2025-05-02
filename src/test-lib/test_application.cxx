#include <erebus/testing/test_application.hxx>
#include <erebus/testing/test_event_listener.hxx>

#if ER_WINDOWS
    #include <erebus/rtl/logger/win32_debugger_sink.hxx>
#endif
#include <erebus/rtl/logger/simple_filter.hxx>
#include <erebus/rtl/logger/simple_formatter.hxx>
#include <erebus/rtl/logger/ostream_sink.hxx>

#include <gtest/gtest.h>

namespace Erp::Testing
{


TestApplication::TestApplication(int options) noexcept
    : Er::Program(options)
{
#if ER_POSIX
    {
        // globally block signals so that child threads 
        // inherit signal mask with signals blocked 
        sigset_t mask;
        ::sigemptyset(&mask);
        ::sigaddset(&mask, SIGTERM);
        ::sigaddset(&mask, SIGHUP);
        ::sigaddset(&mask, SIGINT);
        ::sigaddset(&mask, SIGUSR1);
        ::sigaddset(&mask, SIGUSR2);
        ::sigprocmask(SIG_BLOCK, &mask, nullptr);
    }
#endif
}

void TestApplication::addLoggers(Er::Log::ITee* main)
{
    Er::Log::SimpleFormatter::Options formatOptions = Er::Log::g_verbose ?
    
    Er::Log::SimpleFormatter::Options{
        Er::Log::SimpleFormatter::Option::Option::Time,
        Er::Log::SimpleFormatter::Option::Option::Level,
        Er::Log::SimpleFormatter::Option::Option::Tid,
        Er::Log::SimpleFormatter::Option::Option::TzLocal,
        Er::Log::SimpleFormatter::Option::Option::Lf,
        Er::Log::SimpleFormatter::Option::Option::Component
    } : 
    Er::Log::SimpleFormatter::Options{ 
        Er::Log::SimpleFormatter::Option::Lf 
    };

#if ER_WINDOWS
    if (Er::isDebuggerPresent())
    {
        auto sink = Er::Log::makeDebuggerSink(
            Er::Log::makeSimpleFormatter(formatOptions),
            Er::Log::FilterPtr{}
        );

        main->addSink("debugger", sink);
    }
#endif

    {
        auto sink = Er::Log::makeOStreamSink(
            std::cout,
            Er::Log::makeSimpleFormatter(formatOptions),
            Er::Log::makeLevelFilter(Er::Log::Level::Debug, Er::Log::Level::Info)
        );

        main->addSink("std::cout", sink);
    }

    {
        auto sink = Er::Log::makeOStreamSink(
            std::cerr,
            Er::Log::makeSimpleFormatter(formatOptions),
            Er::Log::makeLevelFilter(Er::Log::Level::Warning)
        );

        main->addSink("std::cerr", sink);
    }
}

int TestApplication::run(int argc, char** argv)
{
#if ER_POSIX
    // unblock signals in the test thread
    {
        sigset_t mask;
        ::sigemptyset(&mask);
        ::sigaddset(&mask, SIGTERM);
        ::sigaddset(&mask, SIGINT);
        ::sigaddset(&mask, SIGUSR1);
        ::sigaddset(&mask, SIGUSR2);
        ::sigprocmask(SIG_UNBLOCK, &mask, nullptr);
    }
#endif
    ::testing::InitGoogleTest(&argc, argv);

    auto& listeners = testing::UnitTest::GetInstance()->listeners();
    delete listeners.Release(listeners.default_result_printer());
    listeners.Append(new TestEventListener);

    return RUN_ALL_TESTS();
}


} // Erp::Testing{}