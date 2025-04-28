#pragma once

#include <erebus/ipc/grpc/client/iclient.hxx>
#include <erebus/ipc/grpc/client/isystem_info_client.hxx>

namespace Er::Ipc::Grpc
{
    
ISystemInfoClient::Ptr createSystemInfoClient(ChannelPtr channel, Log::ILogger::Ptr log);
    
} // namespace Er::Ipc::Grpc {}