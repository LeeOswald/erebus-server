#pragma once

#include <erebus/mutexpool.hxx>

namespace Er
{


// 
// object that has its own mutex
//

template <typename T, Mutex MutexT>
class LockableObject
    : public T
{
public:
    using Base = T;
    using MutexType = MutexT;

    LockableObject() = default;

    template <typename... Args>
    LockableObject(MutexType& mutex, Args&&... args)
        : T(std::forward<Args>(args)...)
    {
    }

    // copying and moving are senseless for lockable objects
    LockableObject(const LockableObject&) = delete;
    LockableObject& operator=(const LockableObject&) = delete;

    LockableObject(LockableObject&&) = delete;
    LockableObject& operator=(LockableObject&&) = delete;

    MutexType& mutex() const noexcept
    {
        return m_mutex;
    }

private:
    mutable MutexType m_mutex;
};


// 
// object that has an external mutex (suitable for use with a mutex pool)
//

template <typename T, Mutex MutexT>
class ExternallyLockableObject
    : public T
{
public:
    using Base = T;
    using MutexType = MutexT;

    ExternallyLockableObject() = default;

    template <typename... Args>
    ExternallyLockableObject(MutexType& mutex, Args&&... args)
        : T(std::forward<Args>(args)...)
        , m_mutex(mutex)
    {
    }

    // copying and moving are senseless for lockable objects
    ExternallyLockableObject(const ExternallyLockableObject&) = delete;
    ExternallyLockableObject& operator=(const ExternallyLockableObject&) = delete;

    ExternallyLockableObject(ExternallyLockableObject&&) = delete;
    ExternallyLockableObject& operator=(ExternallyLockableObject&&) = delete;

    MutexType& mutex() const noexcept
    {
        return m_mutex;
    }

private:
    MutexType& m_mutex;
};


template <typename T>
concept Lockable =
    requires(const T const_instance)
    {
        { const_instance.mutex() };
        { const_instance.mutex().lock() };
        { const_instance.mutex().unlock() };
    };


template <typename T>
concept SharedlyLockable =
    requires(const T const_instance)
    {
        { const_instance.mutex() };
        { const_instance.mutex().lock_shared() };
        { const_instance.mutex().unlock_shared() };
    };


//
// acquire an exclusive access to the underlying object
//

template <Lockable ObjectT>
class ObjectLock
    : public NonCopyable
{
public:
    using ObjectType = ObjectT;

    ~ObjectLock()
    {
        m_object->mutex().unlock();
    }

    explicit ObjectLock(ObjectType& o) noexcept
        : m_object(&o)
    {
        m_object->mutex().lock();
    }

    explicit ObjectLock(ObjectType* o) noexcept
        : m_object(o)
    {
        ErAssert(m_object);
        m_object->mutex().lock();
    }

    [[nodiscard]] constexpr const ObjectType* get() const noexcept
    {
        return m_object;
    }

    [[nodiscard]] constexpr const ObjectType* get() noexcept
    {
        return m_object;
    }

    [[nodiscard]] constexpr const ObjectType* operator->() const noexcept
    {
        ErAssert(m_object);
        return m_object;
    }

    [[nodiscard]] constexpr ObjectType* operator->() noexcept
    {
        ErAssert(m_object);
        return m_object;
    }

private:
    ObjectType* m_object = nullptr;
};


//
// acquire a shared access to the underlying object
//

template <SharedlyLockable ObjectT>
class SharedObjectLock
    : public NonCopyable
{
public:
    using ObjectType = ObjectT;

    ~SharedObjectLock()
    {
        m_object->mutex().unlock_shared();
    }

    explicit SharedObjectLock(const ObjectType& o) noexcept
        : m_object(&o)
    {
        m_object->mutex().lock_shared();
    }

    explicit SharedObjectLock(const SharedObjectLock* o) noexcept
        : m_object(o)
    {
        ErAssert(m_object);
        m_object->mutex().lock_shared();
    }

    [[nodiscard]] constexpr const ObjectType* get() const noexcept
    {
        return m_object;
    }

    [[nodiscard]] constexpr const ObjectType* operator->() const noexcept
    {
        ErAssert(m_object);
        return m_object;
    }

private:
    // shared access implies read-only
    const ObjectType* m_object = nullptr;
};


} // namespace Er {}