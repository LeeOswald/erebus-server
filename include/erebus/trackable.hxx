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
        Cleaned
    };

    Trackable() = default;

    template <typename... Args>
    constexpr Trackable(bool existing, TimePoint t, Args&&... args)
        : Base(std::forward<Args&&>(args)...)
        , m_state(existing ? State::Existing : State::New)
        , m_timeChecked(t)
        , m_timeTracked(t)
        , m_timeModified(t)
    {
    }

    // copying has no meaning with trackable items
    Trackable(const Trackable&) = delete;
    Trackable& operator=(const Trackable&) = delete;

    static TimePoint now()
    {
        return Clock::now();
    }

    constexpr State state() const noexcept
    {
        return m_state;
    }

    TimePoint timeModified() const noexcept
    {
        return m_timeModified;
    }

    void updateTimeModified(TimePoint t = now())
    {
        m_timeModified = t;
    }

    void updateTimeChecked(TimePoint t = now())
    {
        m_timeChecked = t;
    }

    template <class ContainerT, typename GetItemF, typename UpdateItemF>
        requires std::is_invocable_v<GetItemF, typename ContainerT::iterator> &&
                std::is_invocable_r_v<typename ContainerT::iterator, UpdateItemF, typename ContainerT::iterator, State, State>
    static void trackNewDeleted(ContainerT& container, TimePoint now, std::chrono::milliseconds trackThreshold, GetItemF getItem, UpdateItemF updateItem)
    {
        for (auto it = container.begin(); it != container.end();)
        {
            auto item = getItem(it);
            auto prevState = item->state();

            if (item->m_timeChecked < now)
            {
                // timestamp hasn't been updated: this item has died
                if ((item->state() == State::Deleted) && (item->m_timeTracked < now - trackThreshold))
                {
                    // the item has been tracked as deleted for quite a long; time to finally remove it
                    item->clean();
                    it = updateItem(it, State::Cleaned, prevState);
                    continue;
                }
                else
                {
                    // start tracking this item as deleted
                    if (item->trackDeleted(now))
                    {
                        updateItem(it, State::Deleted, prevState);
                    }
                }
            }
            else
            {
                // the item is still alive
                if (item->state() == State::New)
                {
                    if (item->m_timeTracked < now - trackThreshold)
                    {
                        // stop tracking this item as new
                        item->stopTrackingNew();
                        updateItem(it, State::Existing, prevState);
                    }
                }
            }

            ++it;
        }
    }

private:
    void stopTrackingNew()
    {
        m_state = State::Existing;
        m_timeTracked = Never;
    }

    bool trackDeleted(TimePoint t = now())
    {
        if (m_state != State::Deleted)
        {
            m_state = State::Deleted;
            m_timeTracked = t;
            return true;
        }

        return false;
    }

    void clean()
    {
        m_state = State::Cleaned;
    }

    State m_state = State::Undefined;
    TimePoint m_timeChecked = Never;
    TimePoint m_timeTracked = Never;
    TimePoint m_timeModified = Never;
};
    
    
} // namespace Er{}    