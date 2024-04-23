#include "common.hpp"

#include <erebus/log.hxx>

#include <iostream>
#include <sstream>

#include <boost/stacktrace.hpp>

#if ER_DEBUG && defined(_MSC_VER)
#include <crtdbg.h>
#endif

class Logger
    : public Er::Log::LogBase
{
public:
    ~Logger()
    {
        removeDelegate("this");
    }

    explicit Logger(Er::Log::Level level)
        : Er::Log::LogBase(level, 65536)
    {
        addDelegate("this", [this](std::shared_ptr<Er::Log::Record> r) { delegate(r); });
        unmute();
    }

private:
    void delegate(std::shared_ptr<Er::Log::Record> r)
    {
        std::lock_guard l(m_mutex);

        if (r->level < Er::Log::Level::Warning)
            std::cout << r->message << std::endl;
        else 
            std::cerr << r->message << std::endl;
    }

    std::mutex m_mutex;
};

void terminateHandler()
{
    std::ostringstream ss;
    ss << boost::stacktrace::stacktrace();

    std::cerr << "std::terminate() called from\n" << ss.str();

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
#endif

#if ER_WINDOWS
    ::SetConsoleOutputCP(CP_UTF8);
#endif

    ::testing::InitGoogleTest(&argc, argv);


    Logger log(Er::Log::Level::Debug);
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

    auto ret = RUN_ALL_TESTS();

    return ret;
}

