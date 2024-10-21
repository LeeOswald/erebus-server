#include <erebus/debug.hxx>
#include <erebus/errno.hxx>

#if ER_WINDOWS
#include <erebus/util/utf16.hxx>
#endif

#include <iostream>
#include <fstream>
#include <syncstream>

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

#if ER_WINDOWS

static void trace_impl(const std::string& text)
{
    auto textu16 = Er::Util::utf8ToUtf16(text);
    textu16.append(L"\n");

    ::OutputDebugStringW(textu16.c_str());
}

#else

static void trace_impl(const std::string& text)
{
    std::osyncstream(std::cout) << text << "\n";
}

#endif


EREBUS_EXPORT void trace(std::string&& message) noexcept
{
#if ER_WINDOWS
    if (!::IsDebuggerPresent())
        return;
#endif

    trace_impl(message);
}

} // namespace Er {}