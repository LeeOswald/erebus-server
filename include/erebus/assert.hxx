#pragma once

#include <erebus/debug.hxx>
#include <erebus/location.hxx>


#if !ER_ENABLE_ASSERT
    #if ER_DEBUG
        #define ER_ENABLE_ASSERT 1
    #endif
#endif


namespace Er
{

using PrintFailedAssertionFn = void(*)(std::string_view message);


EREBUS_EXPORT void setPrintFailedAssertionFn(PrintFailedAssertionFn f) noexcept;
EREBUS_EXPORT void printFailedAssertion(Location&& location, const char* expression) noexcept;

} // namespace Er {}


#if ER_ENABLE_ASSERT

#define ErAssert(expr) \
    do \
    { \
        if (!bool(expr)) \
        { \
            ::Er::printFailedAssertion( \
                ::Er::Location(std::source_location::current(), ::Er::StackTrace(0, static_cast<std::size_t>(-1))), \
                #expr \
            ); \
            if (::Er::isDebuggerPresent()) \
                _ER_TRAP(); \
            else \
                std::abort(); \
        } \
    } while (false)


#else

#define ErAssert(expr) \
    static_cast<void>(0)

#endif

