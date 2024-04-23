#include <erebus/system/timestamp.hxx>

#if ER_POSIX
#include <time.h>
#endif

namespace Er
{

namespace System
{

#if ER_WINDOWS

namespace
{

int64_t resolutionImpl() noexcept
{
    LARGE_INTEGER freq = {};
    ::QueryPerformanceFrequency(&freq);
    return freq.QuadPart;
}

} // namespace {}

Timestamp Timestamp::now() noexcept
{
    LARGE_INTEGER time = {};
    ::QueryPerformanceCounter(&time);
    return Timestamp(time.QuadPart);
}

int64_t Timestamp::resolution() noexcept
{
    static int64_t s_resolution = resolutionImpl();
    return s_resolution;
}

#elif ER_POSIX

Timestamp Timestamp::now() noexcept
{
    struct timespec time = {};
    ::clock_gettime(CLOCK_MONOTONIC, &time);
    auto v = time.tv_sec;
    v *= 1000000000LL;
    v += time.tv_nsec / 100;
    return Timestamp(v);
}

int64_t Timestamp::resolution() noexcept
{
    return 10000000LL; //100 ns
}

#endif


} // namespace System {}

} // namespace Er {}