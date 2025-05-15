#pragma once

#include <erebus/ipc/grpc/server/iserver.hxx>
#include <erebus/rtl/log.hxx>
#include <erebus/rtl/property_bag.hxx>

#if ER_WINDOWS
    #ifdef ER_GRPC_SERVER_EXPORTS
        #define ER_GRPC_SERVER_EXPORT __declspec(dllexport)
    #else
        #define ER_GRPC_SERVER_EXPORT __declspec(dllimport)
    #endif
#else
    #define ER_GRPC_SERVER_EXPORT __attribute__((visibility("default")))
#endif



namespace Er::Ipc::Grpc
{

ER_GRPC_SERVER_EXPORT [[nodiscard]] ServerPtr createServer(const PropertyMap& parameters, Log::LoggerPtr log);

ER_GRPC_SERVER_EXPORT [[nodiscard]] ServicePtr createSystemInfoService(Log::LoggerPtr log);

} // namespace Er::Ipc::Grpc {}