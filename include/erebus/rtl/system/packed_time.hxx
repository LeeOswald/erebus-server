#pragma once

#include <erebus/rtl/rtl.hxx>

#include <ctime>
#if ER_WINDOWS
    #include <erebus/rtl/system/unwindows.h>
#elif ER_POSIX
    #include <sys/time.h>
#endif

#include <boost/functional/hash.hpp>


namespace Er::System
{

//
// microseconds since 1970-Jan-01
//

class ER_RTL_EXPORT PackedTime final
{
public:
    using ValueType = std::uint64_t;
    
    constexpr ValueType value() const noexcept
    {
        return m_value;
    }

    constexpr PackedTime(ValueType value  = {}) noexcept
        : m_value(value)
    {
    }

    constexpr bool operator==(const PackedTime& o) const noexcept
    {
        return m_value == o.m_value;
    }

    constexpr auto operator<=>(const PackedTime& o) const noexcept
    {
        return m_value <=> o.m_value;
    }

    auto hash() const noexcept
    {
        boost::hash<decltype(m_value)> h;
        return h(m_value);
    }

    [[nodiscard]] static constexpr PackedTime fromPosixTime(std::time_t t) noexcept
    {
        return { static_cast<ValueType>(t) * 1000000UL };
    }

    template <std::integral Seconds>
    [[nodiscard]] static constexpr PackedTime fromSeconds(Seconds t) noexcept
    {
        return { static_cast<ValueType>(t) * 1000000UL };
    }

    template <std::integral Millieconds>
    [[nodiscard]] static constexpr PackedTime fromMilliseconds(Millieconds t) noexcept
    {
        return { static_cast<ValueType>(t) * 1000UL };
    }

    template <std::integral Microseconds>
    [[nodiscard]] static constexpr PackedTime fromMicroseconds(Microseconds t) noexcept
    {
        return { static_cast<ValueType>(t) };
    }

    [[nodiscard]] static ValueType now() noexcept
    {
#if ER_WINDOWS
        FILETIME ft;
        ::GetSystemTimePreciseAsFileTime(&ft);
        // shift is difference between 1970-Jan-01 & 1601-Jan-01
        // in 100-nanosecond units
        const std::uint64_t shift = 116444736000000000ULL; // (27111902 << 32) + 3577643008
        // 100-nanos since 1601-Jan-01
        std::uint64_t packed = (static_cast<std::uint64_t>(ft.dwHighDateTime) << 32) | static_cast<std::uint64_t>(ft.dwLowDateTime);
        packed -= shift; // filetime is now 100-nanos since 1970-Jan-01
        return (packed / 10U); // truncate to microseconds
#elif ER_POSIX
        timeval tv;
        ::gettimeofday(&tv, nullptr);

        return  ValueType(tv.tv_sec) * 1000000UL + ValueType(tv.tv_usec);
#endif
    }

    [[nodiscard]] std::tm toLocalTime() const noexcept;
    [[nodiscard]] std::tm toUtc() const noexcept;

    [[nodiscard]] constexpr std::time_t toPosixTime() const noexcept
    {
        return m_value / 1000000UL;
    }

    [[nodiscard]] constexpr std::uint64_t toMicroseconds() const noexcept
    {
        return m_value;
    }

    [[nodiscard]] constexpr std::uint64_t toMilliseconds() const noexcept
    {
        return m_value / 1000UL;
    }

    [[nodiscard]] constexpr std::uint64_t toSeconds() const noexcept
    {
        return m_value / 1000000UL;
    }

    [[nodiscard]] constexpr std::uint32_t subSecond() const noexcept
    {
        return static_cast<std::uint32_t>(m_value % 1000000UL);
    }

private:
    ValueType m_value;
};


inline auto hash_value(const PackedTime& t) noexcept
{
    return t.hash();
}

} // namespace Er::System {}
