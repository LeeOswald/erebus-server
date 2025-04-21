#pragma once

#include <erebus/rtl/rtl.hxx>

#if ER_WINDOWS
    #ifdef ER_PROCETREE_EXPORTS
        #define ER_PROCETREE_EXPORT __declspec(dllexport)
    #else
        #define ER_PROCETREE_EXPORT __declspec(dllimport)
    #endif
#else
    #define ER_PROCETREE_EXPORT __attribute__((visibility("default")))
#endif