#pragma once

#include <erebus/rtl/log.hxx>


#ifndef ER_GRPC_SERVER_TRACE
    #if ER_DEBUG
        #define ER_GRPC_SERVER_TRACE 1
    #else
        #define ER_GRPC_SERVER_TRACE 0
    #endif
#endif



#if ER_GRPC_SERVER_TRACE


#define ServerTrace2(sink, format, ...) \
    if (sink->level() <= ::Er::Log::Level::Debug) \
        ::Er::Log::debug(sink, format, ##__VA_ARGS__)


#define ServerTraceIndent2(sink, format, ...) \
    if (sink->level() <= ::Er::Log::Level::Debug) \
        ::Er::Log::write(sink, ::Er::Log::Level::Debug, format, ##__VA_ARGS__); \
    ::Er::Log::IndentScope __ids(sink, ::Er::Log::Level::Debug)

#else // !ER_GRPC_SERVER_TRACE

#define ServerTrace2(sink, format, ...)              ((void)0)

#define ServerTraceIndent2(sink, format, ...)        ((void)0)

#endif // !ER_GRPC_SERVER_TRACE