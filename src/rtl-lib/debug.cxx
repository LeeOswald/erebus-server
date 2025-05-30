#include <erebus/rtl/debug.hxx>
#include <erebus/rtl/util/errno.hxx>

#include <fstream>

#if ER_WINDOWS
    #include <erebus/rtl/system/unwindows.h>
#endif

#if ER_POSIX
    #include <erebus/rtl/util/errno.hxx>
#endif

namespace Er
{


#if ER_WINDOWS

ER_RTL_EXPORT bool isDebuggerPresent() noexcept
{
    return !!::IsDebuggerPresent();
}

#elif ER_LINUX

ER_RTL_EXPORT bool isDebuggerPresent() noexcept
{
    try
    {
        Er::Util::ErrnoGuard guard;

        std::ifstream in("/proc/self/status");
        for (std::string line; std::getline(in, line); )
        {
            static const int PREFIX_LEN = 11;
            if (line.compare(0, PREFIX_LEN, "TracerPid:\t") == 0)
            {
                return (line.length() > PREFIX_LEN) && (line[PREFIX_LEN] != '0');
            }
        }
    }
    catch (...)
    {
        // nothing we can do here
    }

    return false;
}

#else

ER_RTL_EXPORT bool isDebuggerPresent() noexcept
{
    return false;
}

#endif

} // namespace Er {}