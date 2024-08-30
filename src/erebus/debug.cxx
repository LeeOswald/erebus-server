#include <erebus/debug.hxx>
#include <erebus/errno.hxx>
#include <erebus/syncstream.hxx>

#if ER_WINDOWS
#include <erebus/util/utf16.hxx>
#endif

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
    Er::osyncstream(std::cout) << text << "\n";
}

#endif

static void tracev(const char* format, va_list args) noexcept
{
    try
    {
        va_list args1;
        va_copy(args1, args);
        auto required = ::vsnprintf(nullptr, 0, format, args1);

        std::string formatted;
        formatted.resize(required);
        ::vsnprintf(formatted.data(), required + 1, format, args);

        va_end(args1);

        return trace_impl(formatted);
    }
    catch (...)
    {
    }
}

EREBUS_EXPORT void trace(const char* format, ...) noexcept
{
#if ER_WINDOWS
    if (!::IsDebuggerPresent())
        return;
#endif

    if (!format || !*format)
        return;

    va_list args;
    va_start(args, format);

    tracev(format, args);

    va_end(args);
}

} // namespace Er {}