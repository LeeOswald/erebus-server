#include "common.hpp"

#include <erebus/program.hxx>

int InstanceCounter::instances = 0;


namespace
{

class TestApplication final
    : public Er::Program
{
public:
    TestApplication() noexcept = default;

private:
    void addCmdLineOptions(boost::program_options::options_description& options) override
    {
        options.add_options()
            ("gtest_list_tests", "list all tests")
            ("gtest_filter", "run specific tests")
            ;
    }

    void addLoggers(Er::Log::ILog* logger) override
    {
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
    }

    bool doInitialize() override
    {
        TestProps::registerAll(log());

        return true;
    }

    int doRun(int argc, char** argv) override
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

        return RUN_ALL_TESTS();
    }

    void doFinalize() noexcept override
    {
        TestProps::unregisterAll(log());
    }
};

} // namespace {}


int main(int argc, char** argv)
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

    TestApplication::globalStartup(argc, argv, Er::Program::NoSignalWaiter);
    TestApplication app;

    auto resut = app.run(argc, argv);

    TestApplication::globalShutdown();
    return resut;
}

