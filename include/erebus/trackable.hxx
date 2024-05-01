#pragma once

#include <erebus/erebus.hxx>

#include <chrono>

namespace Er
{
    
//    
// track object creation/deletion time; useful for displaying them in UI,
// e.g., visualizing spawning and terminating processes
//

template <typename T>
class Trackable
    : public T
{
public:
    using Base = T;
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    static constexpr const TimePoint Never = TimePoint();

    enum class State
    {
        Undefined,
        New,
        Existing,
        Deleted,
        Purged
    };

    Trackable() = default;

    template <typename... Args>
    constexpr Trackable(bool existing, TimePoint t, Args&&... args)
        : Base(std::forward<Args&&>(args)...)
        , m_state(existing ? State::Existing : State::New)
        , m_timeTracked(t)
    {
    }

    // copying has no meaning for trackable items
    Trackable(const Trackable&) = delete;
    Trackable& operator=(const Trackable&) = delete;

    static TimePoint now() noexcept
    {
        return Clock::now();
    }

    constexpr State state() const noexcept
    {
        return m_state;
    }

    bool isTracked() const noexcept
    {
        return (m_timeTracked != Never);
    }

    TimePoint timeTracked() const noexcept
    {
        return m_timeTracked;
    }

    void markDeleted(TimePoint t) noexcept
    {
        ErAssert(m_state != State::Deleted);
        
        m_state = State::Deleted;
        m_timeTracked = t;
    }

    bool maybeUntrackDeleted(TimePoint now, std::chrono::milliseconds trackThreshold) noexcept
    {
        if (m_state == State::Deleted)
        {
            if (m_timeTracked < now - trackThreshold)
            {
                m_state = State::Purged;
                m_timeTracked = Never;
                return true;
            }
        }

        return false;
    }

    bool maybeUntrackNew(TimePoint now, std::chrono::milliseconds trackThreshold) noexcept
    {
        if (m_state == State::New)
        {
            if (m_timeTracked < now - trackThreshold)
            {
                m_state = State::Existing;
                m_timeTracked = Never;
                return true;
            }
        }

        return false;
    }

private:
    State m_state = State::Undefined;
    TimePoint m_timeTracked = Never;  // time this item has been being tracked since
};


    
} // namespace Er{}    