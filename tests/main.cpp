#include "common.hpp"

#include <erebus/log.hxx>

#include <iostream>

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

void signalHandler(int signo)
{
    std::cerr << "Signal " << signo << "\n";
    while (!::IsDebuggerPresent())
        ::Sleep(100);
    __debugbreak();
}

int main(int argc, char** argv)
{
#if ER_DEBUG && defined(_MSC_VER)
    int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(tmpFlag);
#endif

#if ER_WINDOWS
    ::SetConsoleOutputCP(CP_UTF8);
#endif

    ::signal(SIGABRT, signalHandler);

    ::testing::InitGoogleTest(&argc, argv);

    Logger log(Er::Log::Level::Debug);
    Er::LibScope er(&log);

    auto ret = RUN_ALL_TESTS();

    return ret;
}

