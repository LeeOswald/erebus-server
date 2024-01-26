#pragma once

#include <erebus/platform.hxx>


#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef EREBUS_EXPORTS
        #define EREBUS_EXPORT __declspec(dllexport)
    #else
        #define EREBUS_EXPORT __declspec(dllimport)
    #endif
#else
    #define EREBUS_EXPORT __attribute__((visibility("default")))
#endif


namespace Er
{

EREBUS_EXPORT void initialize();
EREBUS_EXPORT void finalize();

class Scope
    : public boost::noncopyable
{
public:
    ~Scope()
    {
        finalize();
    }

    Scope()
    {
        initialize();
    }
};


enum class CallbackResult
{
    Continue,
    Abort
};

} // namespace Er {}
