#include "common.hpp"

#include <erebus/condition.hxx>

#include <chrono>
#include <future>
#include <thread>


TEST(Condition, get_set)
{
    Er::Condition<int> condition(23);
    EXPECT_EQ(condition.get(), 23);
    
    condition.set(42);
    EXPECT_EQ(condition.get(), 42);
    
    condition.setAndNotifyAll(1);
    EXPECT_EQ(condition.get(), 1);
    
    condition.setAndNotifyOne(2);
    EXPECT_EQ(condition.get(), 2);
}

TEST(Condition, wait)
{
    Er::Condition condition(false);
    
    std::future<bool> future =
        std::async(
            std::launch::async,
            [&condition]()
            { 
                condition.wait(); 
                return true; 
            }
        );

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::this_thread::yield();

    condition.notifyOne();
    EXPECT_TRUE(future.get());
}

TEST(Condition, wait_pred)
{
    Er::Condition condition(false);
    
    std::future<bool> future =
        std::async(
            std::launch::async,
            [&condition]()
            { 
                condition.wait(
                    []()
                    { 
                        return true; 
                    }
                ); 
                return true; 
            }
        );

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::this_thread::yield();

    condition.notifyOne();
    EXPECT_TRUE(future.get());
}

TEST(Condition, waitValue)
{
    Er::Condition condition(0);
    
    std::future<int> future =
        std::async(
            std::launch::async,
            [&condition]()
            { 
                condition.waitValue(23); 
                return condition.get(); 
            }
        );

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::this_thread::yield();

    condition.notifyAll();
    condition.setAndNotifyOne(23);

    EXPECT_EQ(future.get(), 23);
}

TEST(Condition, waitFor_predicate)
{
    Er::Condition condition(23);
    
    auto pred = [&condition](){ return condition.get() == 42; };
    
    std::future<void> future =
        std::async(
            std::launch::async,
            [&condition, &pred]()
            {
                condition.waitFor(std::chrono::hours(1000), pred); 
            }
        );

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::this_thread::yield();

    condition.set(42);
    condition.notifyAll();
    future.wait();

    EXPECT_EQ(condition.get(), 42);
}

TEST(Condition, waitValueFor)
{
    Er::Condition condition(23);
    
    std::future<void> future =
        std::async(
            std::launch::async,
            [&condition]()
            {
                condition.waitValueFor(42, std::chrono::hours(1000)); 
            }
        );

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::this_thread::yield();

    condition.setAndNotifyAll(42);
    future.wait();

    EXPECT_EQ(condition.get(), 42);
}

TEST(Condition, waitFor_predicate_simple)
{
    Er::Condition condition(23);
    
    auto pred = [&condition](){ return condition.get() == 42; };
    
    std::future<void> future =
        std::async(
            std::launch::async,
            [&condition, &pred]()
            {
                condition.waitFor(std::chrono::hours(1000), pred); 
            }
        );

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::this_thread::yield();

    condition.setAndNotifyAll(42);
    future.wait();

    EXPECT_EQ(condition.get(), 42);
}

TEST(Condition, waitUntil_predicate)
{
    Er::Condition condition(23);

    auto pred = [&condition](){ return condition.get() == 42; };

    Er::Condition<bool> fence(false);

    std::future<void> future =
        std::async(
            std::launch::async,
            [&condition, &pred, &fence]()
            {
                auto duration = std::chrono::system_clock::now() + std::chrono::hours(1);
                fence.setAndNotifyAll(true);
                condition.waitUntil(duration, pred); 
            }
        );

    fence.waitValue(true);
    EXPECT_TRUE(fence.get());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::this_thread::yield();

    condition.set(42);
    condition.notifyAll();
    future.wait();

    EXPECT_EQ(condition.get(), 42);
}

TEST(Condition, waitValueUntil)
{
    Er::Condition condition(23);

    std::future<int> future =
        std::async(
            std::launch::async,
            [&condition]()
            {
                condition.waitValueUntil(42, std::chrono::system_clock::now() + std::chrono::hours(1));
                return condition.get();
            }
        );

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::this_thread::yield();

    condition.notifyAll();
    condition.set(42);
    condition.notifyAll();

    EXPECT_EQ(future.get(), 42);
    EXPECT_EQ(condition.get(), 42);
}
