#pragma once

#include <erebus/erebus.hxx>


namespace Er
{

//
// we need something like 'struct tm' but with milliseconds field
//

struct EREBUS_EXPORT Time final
{
    uint16_t year = 0;
    uint16_t month = 0;
    uint16_t day = 0;
    uint16_t hour = 0;
    uint16_t minute = 0;
    uint16_t second = 0;
    uint16_t milli = 0;

    constexpr Time() noexcept = default;
    
    explicit constexpr Time(uint16_t year, uint16_t month, uint16_t day, uint16_t hour, uint16_t minute, uint16_t second, uint16_t milli) noexcept
        : year(year)
        , month(month)
        , day(day)
        , hour(hour)
        , minute(minute)
        , second(second)
        , milli(milli)
    {
    }

    constexpr Time(const Time&) noexcept = default;
    constexpr Time& operator=(const Time&) noexcept = default;

    static Time local() noexcept;
    static Time gmt() noexcept;
};



} // namespace Er {}
