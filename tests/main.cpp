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

class Logger
    : public Er::Log::LogBase
{
public:
    ~Logger()
    {
        Er::Log::LogBase::flush();
        Er::Log::LogBase::removeDelegate("this");
    }

    explicit Logger(Er::Log::Level level)
        : Er::Log::LogBase(Er::Log::LogBase::SyncLog, level)
    {
        Er::Log::LogBase::addDelegate("this", [this](std::shared_ptr<Er::Log::Record> r) { delegate(r); });
        Er::Log::LogBase::unmute();
    }

private:
    void delegate(std::shared_ptr<Er::Log::Record> r)
    {
        if (r->level < Er::Log::Level::Warning)
            Er::osyncstream(std::cout) << r->message << std::endl;
        else 
            Er::osyncstream(std::cerr) << r->message << std::endl;

#if ER_WINDOWS && ER_DEBUG
        if (::IsDebuggerPresent())
            ::OutputDebugStringW(Er::Util::utf8ToUtf16(r->message).append(L"\n").c_str());
#endif
    }
};

void terminateHandler()
{
    std::ostringstream ss;
    ss << boost::stacktrace::stacktrace();

    Er::osyncstream(std::cerr) << "std::terminate() called from\n" << ss.str();

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
        Logger log(Er::Log::Level::Debug);
        g_log = &log;
        Er::LibScope er(&log);


#if ER_POSIX
        // unblock signals in the test thread
        ::sigemptyset(&mask);
        ::sigaddset(&mask, SIGTERM);
        ::sigaddset(&mask, SIGINT);
        ::sigaddset(&mask, SIGUSR1);
        ::sigaddset(&mask, SIGUSR2);
        ::sigprocmask(SIG_UNBLOCK, &mask, nullptr);
#endif

        TestProps::registerAll(&log);
    
        ret = RUN_ALL_TESTS();

        TestProps::unregisterAll(&log);
        g_log = nullptr;
    }
    catch (std::exception& e)
    {
        Er::osyncstream(std::cerr) << "Unexpected error: " << e.what() << "\n";
    }

    return ret;
}

