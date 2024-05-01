#pragma once

#include <erebus/erebus.hxx>

#include <atomic>
#include <vector>

namespace Er
{


template <class MutexT>
concept Mutex = 
    requires(MutexT m)
    {
        { m.lock() };
        { m.unlock() };
    };

template <class MutexT>
concept SharedMutex = 
    requires(MutexT m)
    {
        { m.lock() };
        { m.unlock() };
        { m.lock_shared() };
        { m.unlock_shared() };
    };


//
// mutex pool is useful when we have a huge bunch of objects
// each of which needs to be independently accessed in a thread-safe manner;
// creating one mutex per object is too expensive in this case;
// given that any thread accesses one object at a time most of the time, we can
// maintain a relatively small pool of mutexes and assign them to our objects in a
// round-robin manner so that each mutex protects many objects without a significant performance impact
//


template <class MutexT>
class MutexPool;


template <Mutex MutexT>
class MutexPool<MutexT> final
    : public NonCopyable
{
public:
    using MutexType = MutexT;

    explicit MutexPool(std::size_t count)
        : m_mutices(count)
    {
        ErAssert(count > 0);
    }

    [[nodiscard]] MutexType& mutex() noexcept
    {
        std::size_t index;
        std::size_t expected;
        do
        {
            expected = m_next.load(std::memory_order_relaxed);
            index = expected + 1;
            if (index == m_mutices.size())
                index = 0;

        } while (!m_next.compare_exchange_strong(expected, index, std::memory_order_acq_rel));
        
        return m_mutices[index];
    }

private:
    std::atomic<std::size_t> m_next = 0;
    std::vector<MutexType> m_mutices;
};



} // namespace Er {}