#pragma once

#include <erebus/ipc/grpc/client/iclient.hxx>
#include <erebus/ipc/grpc/client/isystem_info_client.hxx>

namespace Er::Ipc::Grpc
{
    
[[nodiscard]] SystemInfoClientPtr createSystemInfoClient(ChannelPtr channel, Log::LoggerPtr log);
    
} // namespace Er::Ipc::Grpc {}