#include <erebus/erebus.hxx>
#include <erebus/errno.hxx>

#include <fstream>


namespace Er
{


#if ER_WINDOWS

EREBUS_EXPORT bool isDebuggerPresent()
{
    return !!::IsDebuggerPresent();
}

#elif ER_LINUX

EREBUS_EXPORT bool isDebuggerPresent()
{
    ErrnoGuard guard;

    std::ifstream in("/proc/self/status");
    for (std::string line; std::getline(in, line); )
    {
        static const int PREFIX_LEN = 11;
        if (line.compare(0, PREFIX_LEN, "TracerPid:\t") == 0)
        {
            return (line.length() > PREFIX_LEN) && (line[PREFIX_LEN] != '0');
        }
    }

    return false;
}

#else

EREBUS_EXPORT bool isDebuggerPresent()
{
    return false;
}

#endif

} // namespace Er {}