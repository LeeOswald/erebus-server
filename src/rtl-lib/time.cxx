#include <erebus/rtl/time.hxx>



namespace Er
{

std::tm Time::toLocalTime() const noexcept
{
    auto seconds = toPosixTime();
    std::tm now_tm = {};
#if ER_WINDOWS
    ::localtime_s(&now_tm, &seconds);
#elif ER_POSIX
    ::localtime_r(&seconds, &now_tm);
#endif
    return now_tm;
}

std::tm Time::toUtc() const noexcept
{
    auto seconds = toPosixTime();
    std::tm now_tm = {};
#if ER_WINDOWS
    ::gmtime_s(&now_tm, &seconds);
#elif ER_POSIX
    ::gmtime_r(&seconds, &now_tm);
#endif
    return now_tm;
}

} // namespace Er {}