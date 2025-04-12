#pragma once

#ifndef ER_RTL_HXX_INCLUDED
    #include <erebus/rtl/rtl.hxx>
#endif

#if ER_LINUX
    #if defined(__GNUC__) && (defined(__i386) || defined(__x86_64))
        #define _ER_TRAP() asm volatile ("int $3") /* NOLINT */
    #else
        #define _ER_TRAP() ::raise(SIGTRAP)
    #endif
#elif defined(_MSC_VER)
    #define _ER_TRAP() __debugbreak()
#elif defined(__MINGW32__)
    #define _ER_TRAP() ::DebugBreak()
#else
    #define _ER_TRAP() ((void)0)
#endif

#ifndef ER_PLATFORM_HXX_INCLUDED
    #include <erebus/rtl/rtl.hxx>
#endif

namespace Er
{

ER_RTL_EXPORT bool isDebuggerPresent() noexcept;

} // namespace Er {}


