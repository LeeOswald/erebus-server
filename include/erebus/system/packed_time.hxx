#pragma once

#include <erebus/erebus.hxx>

#include <ctime>


namespace Er
{
    
namespace System
{

//
// microseconds since 1970-Jan-01
//

struct EREBUS_EXPORT PackedTime
{
    using ValueType = std::uint64_t;
    
    ValueType value;

    constexpr PackedTime(ValueType value  = {}) noexcept
        : value(value)
    {
    }

    constexpr std::time_t toPosixTime() const noexcept
    {
        return value / 1000000UL;
    }

    static ValueType now() noexcept;

    std::tm toLocalTime() const noexcept;
    std::tm toUtc() const noexcept;

    constexpr std::uint32_t subSecond() const noexcept
    {
        return static_cast<std::uint32_t>(value % 1000000UL);
    }
};

} // namespace System {}

} // namespace Er {}
