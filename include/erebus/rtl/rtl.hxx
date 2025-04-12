#pragma once

#include "../platform.hxx"


#if ER_WINDOWS
    #ifdef ER_RTL_EXPORTS
        #define ER_RTL_EXPORT __declspec(dllexport)
    #else
        #define ER_RTL_EXPORT __declspec(dllimport)
    #endif
#else
    #define ER_RTL_EXPORT __attribute__((visibility("default")))
#endif

#define ER_PLATFORM_HXX_INCLUDED 1
