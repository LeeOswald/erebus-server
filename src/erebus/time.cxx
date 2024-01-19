#include <erebus/time.hxx>

#include <ctime>
#include <time.h>

namespace Er
{

Time Time::local() noexcept
{
#if ER_POSIX
    struct tm localNow = {};
    struct timespec now = {};
    ::clock_gettime(CLOCK_REALTIME, &now);

    // round nanoseconds to milliseconds
    long msec = 0;
    if (now.tv_nsec >= 999500000)
    {
        now.tv_sec++;
        msec = 0;
    }
    else
    {
        msec = (now.tv_nsec + 500000) / 1000000;
    }

    ::localtime_r(&now.tv_sec, &localNow);

    return Time(localNow.tm_year, localNow.tm_mon, localNow.tm_mday, localNow.tm_hour, localNow.tm_min, localNow.tm_sec, msec);

#elif ER_WINDOWS
    SYSTEMTIME time = {};
    ::GetLocalTime(&time);

    return Time(time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
#endif
}

Time Time::gmt() noexcept
{
#if ER_POSIX
    struct tm utcNow = {};
    struct timespec now = {};
    ::clock_gettime(CLOCK_REALTIME, &now);

    // round nanoseconds to milliseconds
    long msec = 0;
    if (now.tv_nsec >= 999500000)
    {
        now.tv_sec++;
        msec = 0;
    }
    else
    {
        msec = (now.tv_nsec + 500000) / 1000000;
    }

    ::gmtime_r(&now.tv_sec, &utcNow);

    return Time(utcNow.tm_year, utcNow.tm_mon, utcNow.tm_mday, utcNow.tm_hour, utcNow.tm_min, utcNow.tm_sec, msec);

#elif ER_WINDOWS
    SYSTEMTIME time = {};
    ::GetSystemTime(&time);

    return Time(time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
#endif
}


} // namespace Er {}