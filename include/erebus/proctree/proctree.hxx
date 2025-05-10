#pragma once

#include <erebus/rtl/rtl.hxx>

#if ER_WINDOWS
    #ifdef ER_PROCTREE_EXPORTS
        #define ER_PROCTREE_EXPORT __declspec(dllexport)
    #else
        #define ER_PROCTREE_EXPORT __declspec(dllimport)
    #endif
#else
    #define ER_PROCTREE_EXPORT __attribute__((visibility("default")))
#endif


namespace Er::ProcessTree
{

using Pid = std::uint64_t;

constexpr Pid InvalidPid = Pid(-1);
constexpr Pid KernelPid = 0;
constexpr Pid KThreadDPid = 2;


} // namespace Er::ProcessTree {}