#include <erebus/exception.hxx>
#include <erebus/system/process.hxx>

#if ER_POSIX
    #include <sys/stat.h>
#endif

#if ER_WINDOWS
    #include <erebus/util/utf16.hxx>
#endif


namespace Er
{

namespace System
{

namespace CurrentProcess
{

EREBUS_EXPORT Pid id() noexcept
{
#if ER_WINDOWS
    return static_cast<Pid>(::GetCurrentProcessId());
#elif ER_POSIX
    return static_cast<Pid>(::getpid());
#endif
}

EREBUS_EXPORT std::string exe()
{
#if ER_WINDOWS
    wchar_t exeFile[MAX_PATH];
    ::GetModuleFileNameW(0, exeFile, _countof(exeFile));
    return Util::utf16To8bit(CP_UTF8, exeFile);
#elif ER_POSIX
    
    struct stat sb = { 0 };
    
#ifdef PATH_MAX
    size_t size = PATH_MAX;
#else
    size_t size = 4096;
#endif

    std::string exe;
    exe.resize(size + 1, '\0');
    auto r = ::readlink("/proc/self/exe", exe.data(), size); // readlink does not append '\0'
    if (r < 0)
    {
        throw Exception(ER_HERE(), "Failed to read /proc/self/exe", ExceptionProps::PosixErrorCode(errno));
    }

    exe.resize(std::strlen(exe.c_str())); // cut extra '\0'

    return exe;
    
#endif
}

} // namespace CurrentProcess {}

} // namespace System {}

} // namespace Er {}