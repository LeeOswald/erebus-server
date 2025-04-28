#pragma once

#include <erebus/ipc/grpc/server/iserver.hxx>
#include <erebus/rtl/log.hxx>
#include <erebus/rtl/property_bag.hxx>


namespace Er::Ipc::Grpc
{

[[nodiscard]] IServer::Ptr createServer(const PropertyMap& parameters, Log::ILogger::Ptr log);

[[nodiscard]] IService::Ptr createSystemInfoService(Log::ILogger::Ptr log);

} // namespace Er::Ipc::Grpc {}