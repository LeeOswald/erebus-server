#pragma once

#include <erebus/rtl/rtl.hxx>

#if ER_WINDOWS
#include <erebus/rtl/system/unwindows.h>
#elif ER_POSIX
    #include <unistd.h>
#endif

namespace Er::System
{

using Pid = uintptr_t;


namespace CurrentProcess
{

[[nodiscard]] inline auto id() noexcept
{
#if ER_WINDOWS
    return static_cast<Pid>(::GetCurrentProcessId());
#elif ER_POSIX
    return static_cast<Pid>(::getpid());
#endif
}

ER_RTL_EXPORT [[nodiscard]] std::string exe();

#if ER_POSIX
ER_RTL_EXPORT void daemonize() noexcept;
#endif

} // namespace CurrentProcess {}

} // namespace Er::System {}