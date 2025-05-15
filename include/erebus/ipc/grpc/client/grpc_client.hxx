#pragma once

#include <erebus/ipc/grpc/client/iclient.hxx>
#include <erebus/ipc/grpc/client/isystem_info_client.hxx>

#if ER_WINDOWS
    #ifdef ER_GRPC_CLIENT_EXPORTS
        #define ER_GRPC_CLIENT_EXPORT __declspec(dllexport)
    #else
        #define ER_GRPC_CLIENT_EXPORT __declspec(dllimport)
    #endif
#else
    #define ER_GRPC_CLIENT_EXPORT __attribute__((visibility("default")))
#endif


namespace Er::Ipc::Grpc
{

using ChannelPtr = std::shared_ptr<grpc::Channel>;

ER_GRPC_CLIENT_EXPORT [[nodiscard]] ChannelPtr createChannel(const PropertyMap& parameters);

ER_GRPC_CLIENT_EXPORT [[nodiscard]] SystemInfoClientPtr createSystemInfoClient(ChannelPtr channel, Log::LoggerPtr log);
    
} // namespace Er::Ipc::Grpc {}