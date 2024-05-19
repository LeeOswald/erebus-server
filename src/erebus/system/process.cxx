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
    
    const size_t size = PATH_MAX;
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

#if ER_POSIX
EREBUS_EXPORT void daemonize() noexcept
{
    // Fork the process and have the parent exit. If the process was started
    // from a shell, this returns control to the user. Forking a new process is
    // also a prerequisite for the subsequent call to setsid().
    auto pid = ::fork();

    if (pid < 0)
        ::exit(EXIT_FAILURE);

    if (pid > 0)
        ::exit(EXIT_SUCCESS);

    // Make the process a new session leader. This detaches it from the terminal.
    if (::setsid() < 0)
        ::exit(EXIT_FAILURE);

    // Close the standard streams. This decouples the daemon from the terminal that started it.
    auto null = ::open("/dev/null", O_RDWR);
    ::dup2(null, STDIN_FILENO);
    ::dup2(null, STDOUT_FILENO);
    ::dup2(null, STDERR_FILENO);
    ::close(null);

    ::signal(SIGCHLD, SIG_IGN);
    ::signal(SIGHUP, SIG_IGN);

    // A second fork ensures the process cannot acquire a controlling terminal.
    pid = ::fork();

    if (pid < 0)
        ::exit(EXIT_FAILURE);

    if (pid > 0)
        ::exit(EXIT_SUCCESS);
}
#endif


} // namespace CurrentProcess {}

} // namespace System {}

} // namespace Er {}