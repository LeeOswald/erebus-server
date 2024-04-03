#pragma once

#include <erebus-srv/plugin.hxx>


#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef ER_PROCESSMGR_EXPORTS
        #define ER_PROCESSMGR_EXPORT __declspec(dllexport)
    #else
        #define ER_PROCESSMGR_EXPORT __declspec(dllimport)
    #endif
#else
    #define ER_PROCESSMGR_EXPORT __attribute__((visibility("default")))
#endif




