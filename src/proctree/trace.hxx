#pragma once

#include <erebus/rtl/log.hxx>


#ifndef ER_PROCTREE_TRACE
    #if ER_DEBUG
        #define ER_PROCTREE_TRACE 1
    #else
        #define ER_PROCTREE_TRACE 0
    #endif
#endif



#if ER_PROCTREE_TRACE


#define ProctreeTrace2(sink, format, ...) \
    if (sink->level() <= ::Er::Log::Level::Debug) \
        ::Er::Log::debug(sink, format, ##__VA_ARGS__)


#define ProctreeTraceIndent2(sink, format, ...) \
    if (sink->level() <= ::Er::Log::Level::Debug) \
        ::Er::Log::write(sink, ::Er::Log::Level::Debug, format, ##__VA_ARGS__); \
    ::Er::Log::IndentScope __ids(sink, ::Er::Log::Level::Debug)

#else // !ER_PROCTREE_TRACE

#define ProctreeTrace2(sink, format, ...)              ((void)0)

#define ProctreeTraceIndent2(sink, format, ...)        ((void)0)

#endif // !ER_PROCTREE_TRACE