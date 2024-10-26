#pragma once

#include <erebus/log.hxx>

#define TraceScope(name) \
    Er::Log::write(Er::Log::defaultLog(), Er::Log::Level::Debug, "[{}]", name); \
    Er::Log::defaultLog()->flush(); \
    Er::Log::Indent __log_indent(Er::Log::defaultLog());  \


#define TraceFunction() \
    Er::Log::write(Er::Log::defaultLog(), Er::Log::Level::Debug, "[{}]", std::source_location::current().function_name()); \
    Er::Log::defaultLog()->flush(); \
    Er::Log::Indent __log_indent(Er::Log::defaultLog());  \


#define TraceMethod(className) \
    Er::Log::write(Er::Log::defaultLog(), Er::Log::Level::Debug, "[{}.{}::{}]", Er::Format::ptr(this), className, std::source_location::current().function_name()); \
    Er::Log::defaultLog()->flush(); \
    Er::Log::Indent __log_indent(Er::Log::defaultLog());  \