#pragma once

#ifndef ER_PLATFORM_HXX_INCLUDED
    #include <erebus/platform.hxx>
#endif

#define ER_RTL_HXX_INCLUDED 1

#if ER_WINDOWS
    #ifdef ER_RTL_EXPORTS
        #define ER_RTL_EXPORT __declspec(dllexport)
    #else
        #define ER_RTL_EXPORT __declspec(dllimport)
    #endif
#else
    #define ER_RTL_EXPORT __attribute__((visibility("default")))
#endif


#include <erebus/rtl/assert.hxx>
#include <erebus/rtl/bool.hxx>