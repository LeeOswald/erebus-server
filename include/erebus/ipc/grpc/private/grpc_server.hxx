#pragma once

#include <erebus/ipc/grpc/igrpc_server.hxx>
#include <erebus/rtl/log.hxx>
#include <erebus/rtl/property_bag.hxx>


namespace Er::Ipc::Grpc
{

[[nodiscard]] IServer::Ptr createServer(const PropertyMap& parameters, Log::ILogger::Ptr log);

} // namespace Er::Ipc::Grpc {}