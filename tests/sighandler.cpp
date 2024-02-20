#include "common.hpp"

#include <erebus/condition.hxx>
#include <erebus/util/signalhandler.hxx>

#include <chrono>
#include <future>
#include <thread>


namespace
{

bool worker(const Er::Condition<bool>& condition)
{
    condition.waitFor(
        std::chrono::hours(1000),
        [&condition]()
        { 
            return condition.get(); 
        }
   );

    return condition.get();
}

int loopingWorker(const Er::Condition<bool>& condition)
{
    int i = 0;
    while (!condition.get())
    {
        condition.waitFor(
            std::chrono::milliseconds(2),
            [&condition]()
            { 
                return condition.get(); 
            }
       );
        
        ++i;
    }

    return i;
}

}

TEST(SignalHandler, wait)
{
    Er::Util::SignalHandler handler({SIGUSR1});
    
    ::kill(::getpid(), SIGUSR1);
  
    EXPECT_EQ(handler.wait(), SIGUSR1);
}

TEST(SignalHandler, waitHandler)
{
    auto pred = [](int)
    {
        return true;
    };

    Er::Util::SignalHandler handler({SIGUSR2});
    
    ::kill(::getpid(), SIGUSR2);
    
    EXPECT_EQ(handler.waitHandler(pred), SIGUSR2);
}

TEST(SignalHandler, asyncWaitHandler)
{
    Er::Util::SignalHandler handler({SIGUSR1,SIGUSR2});

    auto pred = [](int)
    {
        return true;
    };

    auto future = handler.asyncWaitHandler(pred);
    
    ::kill(::getpid(), SIGUSR1);
    
    EXPECT_EQ(future.get(), SIGUSR1);
}

TEST(SignalHandler, asyncWaitHandler_condition)
{
    Er::Util::SignalHandler handler({SIGUSR1});
    Er::Condition<int> condition(0);

    EXPECT_EQ(condition.get(), 0);
    EXPECT_NE(condition.get(), SIGUSR1);

    auto pred = [&condition](int signum)
    {
        condition.setAndNotifyOne(signum);
        return true;
    };

    auto future = handler.asyncWaitHandler(pred);
    ::kill(::getpid(), SIGUSR1);
    
    EXPECT_EQ(future.get(), SIGUSR1);
    EXPECT_EQ(condition.get(), SIGUSR1);
}

TEST(SignalHandler, constructor_thread_blocks_signals)
{
    std::atomic<int> last_signal(0);
    Er::Util::SignalHandler handler({SIGTERM, SIGINT});

    auto pred = [&last_signal](int signum) 
    {
        last_signal.store(signum);
        return signum == SIGINT;
    };

    std::future<int> ft_sig_handler =
    std::async(
        std::launch::async,
        &Er::Util::SignalHandler::waitHandler,
        &handler,
        std::ref(pred)
    );

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::this_thread::yield();

    EXPECT_EQ(::kill(::getpid(), SIGTERM), 0);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::this_thread::yield();

    ASSERT_EQ(last_signal.load(), SIGTERM);

    EXPECT_EQ(::kill(::getpid(), SIGINT), 0);

    EXPECT_EQ(ft_sig_handler.get(), SIGINT);
    EXPECT_EQ(last_signal.load(), SIGINT);
}

TEST(SignalHandler, sleeping_workers_with_exit_condition)
{
    Er::Condition exit_condition(false);
    std::initializer_list<int> signals = {SIGINT, SIGTERM, SIGUSR1, SIGUSR2};
    
    for(auto test_signal : signals)
    {
        exit_condition.set(false);
        auto pred = [&exit_condition, test_signal](int signum) 
        {
            exit_condition.set(true);
            exit_condition.notifyAll();
            return test_signal == signum;
        };

        Er::Util::SignalHandler handler({test_signal});
        std::future<int> ft_sig_handler =
            std::async(
                std::launch::async,
                &Er::Util::SignalHandler::waitHandler,
                &handler,
                std::ref(pred)
            );

        std::vector<std::future<bool>> futures;
        for(int i = 0; i < 50; ++i)
        {
            futures.push_back(
                std::async(
                    std::launch::async,
                    worker,
                    std::ref(exit_condition)
                )
            );
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::this_thread::yield();

        EXPECT_EQ(::kill(::getpid(), test_signal), 0);

        for (auto& future : futures)
            ASSERT_TRUE(future.get());

        EXPECT_EQ(ft_sig_handler.get(), test_signal);
    }
}

TEST(SignalHandler, looping_workers_with_exit_condition)
{
    Er::Condition exit_condition(false);
    std::initializer_list<int> signals = {SIGINT, SIGTERM, SIGUSR1, SIGUSR2};
    for (auto test_signal : signals)
    {
        exit_condition.set(false);
        auto pred = [&exit_condition, test_signal](int signum) 
        {
            exit_condition.set(true);
            exit_condition.notifyAll();
            return test_signal == signum;
        };

        Er::Util::SignalHandler handler({test_signal});
        std::future<int> ft_sig_handler =
            std::async(
                std::launch::async,
                &Er::Util::SignalHandler::waitHandler,
                &handler,
                std::ref(pred)
            );

        std::vector<std::future<int>> futures;
        for(int i = 0; i < 10; ++i)
        {
            futures.push_back(
                std::async(
                    std::launch::async,
                    loopingWorker,
                    std::ref(exit_condition)
                )
            );
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::this_thread::yield();

        EXPECT_EQ(::kill(::getpid(), test_signal), 0);

        for (auto& future : futures)
        {
            // After 100 milliseconds, each worker thread should
            // have looped at least 10 times.
            EXPECT_TRUE(future.get() > 10);
        }

        EXPECT_EQ(ft_sig_handler.get(), test_signal);
    }
}

