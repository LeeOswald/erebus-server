#pragma once

#include <erebus/erebus.hxx>

#include <chrono>

namespace Er
{

template <typename UnitT = std::chrono::milliseconds, typename ClockT = std::chrono::high_resolution_clock>
class Stopwatch
{
public:
    using Unit = UnitT;
    using Clock = ClockT;

    constexpr Stopwatch() noexcept = default;

    void start() noexcept
    {
        m_start = Clock::now();
    }

    void stop() noexcept
    {
        auto now = Clock::now();
        auto dura = now - m_start;
        m_value += std::chrono::duration_cast<Unit>(dura).count();
    }

    constexpr void reset() noexcept
    {
        m_value = 0;
    }

    constexpr auto value() const noexcept
    {
        return Unit(m_value);
    }

    struct Scope
    {
        ~Scope()
        {
            owner.stop();
        }

        Scope(Stopwatch& owner) noexcept
            : owner(owner)
        {
            owner.start();
        }

    private:
        Stopwatch& owner;
    };

private:
    uint64_t m_value = 0;
    Clock::time_point m_start = {};
};

} // namespace Er {}