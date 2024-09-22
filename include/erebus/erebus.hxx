#pragma once

#include <erebus/platform.hxx>

#include <erebus/noncopyable.hxx>

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

namespace Log
{
    struct ILog;
}

EREBUS_EXPORT void initialize(Er::Log::ILog* log);
EREBUS_EXPORT void finalize(Er::Log::ILog* log);

class LibScope
    : public Er::NonCopyable
{
public:
    ~LibScope()
    {
        finalize(m_log);
    }

    LibScope(Er::Log::ILog* log)
        : m_log(log)
    {
        initialize(log);
    }

private:
    Er::Log::ILog* const m_log;
};


template <typename T>
auto saturatingSub(T a, T b) noexcept 
{
   return a > b ? a - b : 0;
}

//
// we need a fixed-size bool
//
enum class Bool: uint8_t 
{   
    False = 0,
    True = 1
};

constexpr Bool False = Bool::False;
constexpr Bool True = Bool::True;


} // namespace Er {}

#include <erebus/assert.hxx>