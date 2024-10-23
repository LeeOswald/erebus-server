#pragma once

#include <erebus/erebus.hxx>

#if ER_DEBUG
#include <erebus/system/thread.hxx>
#endif

namespace Er
{

struct NullMutex
{
    constexpr NullMutex() noexcept = default;

    void lock() noexcept
    {
#if ER_DEBUG
        ErAssert(m_owner == System::Tid{});
        m_owner = System::CurrentThread::id();
#endif
    }

    bool try_lock() noexcept
    {
#if ER_DEBUG
        ErAssert(m_owner == System::Tid{});
        m_owner = System::CurrentThread::id();
#endif
        return true;
    }

    void unlock() noexcept
    {
#if ER_DEBUG
        ErAssert(m_owner == System::CurrentThread::id());
        m_owner = System::Tid{};
#endif
    }

#if ER_DEBUG
private:
    System::Tid m_owner = {};
#endif
};


struct NullSharedMutex
{
    constexpr NullSharedMutex() noexcept = default;

    void lock() noexcept
    {
#if ER_DEBUG
        ErAssert(m_owner == System::Tid{});
        m_owner = System::CurrentThread::id();
#endif
    }

    bool try_lock() noexcept
    {
#if ER_DEBUG
        ErAssert(m_owner == System::Tid{});
        m_owner = System::CurrentThread::id();
#endif
        return true;
    }

    void unlock() noexcept
    {
#if ER_DEBUG
        ErAssert(m_owner == System::CurrentThread::id());
        m_owner = System::Tid{};
#endif
    }

    void lock_shared() noexcept
    {
#if ER_DEBUG
        ErAssert(m_owner == System::Tid{});
        m_owner = System::CurrentThread::id();
#endif
    }

    bool try_lock_shared() noexcept
    {
#if ER_DEBUG
        ErAssert(m_owner == System::Tid{});
        m_owner = System::CurrentThread::id();
#endif
        return true;
    }

    void unlock_shared() noexcept
    {
#if ER_DEBUG
        ErAssert(m_owner == System::CurrentThread::id());
        m_owner = System::Tid{};
#endif
    }

#if ER_DEBUG
private:
    System::Tid m_owner = {};
#endif
};

} // namespace Er {}