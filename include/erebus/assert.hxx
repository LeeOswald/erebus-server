#pragma once

#include <erebus/location.hxx>


#if !ER_ENABLE_ASSERT
    #if ER_DEBUG
        #define ER_ENABLE_ASSERT 1
    #endif
#endif


namespace Er
{

namespace Private
{

EREBUS_EXPORT void failAssert(Location&& location, const char* expression);


template <typename PredT>
void doAssert(Location&& location, const char* expression, PredT pred)
{
    if (!pred())
        failAssert(std::move(location), expression);

}


} // namespace Private {}

} // namespace Er {}


#if ER_ENABLE_ASSERT

#define ErAssert(expr) \
    static_cast<void>(::Er::Private::doAssert( \
        ::Er::Location(::Er::SourceLocationImpl::current(), ::Er::StackTrace(0, static_cast<std::size_t>(-1))), \
        #expr, \
        [&]() noexcept { return expr; } \
    ))


#else

#define ErAssert(expr) \
    static_cast<void>(0)

#endif

