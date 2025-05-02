#pragma once

#include <erebus/ipc/grpc/server/iserver.hxx>
#include <erebus/rtl/log.hxx>
#include <erebus/rtl/property_bag.hxx>


namespace Er::Ipc::Grpc
{

[[nodiscard]] IServer* createServer(const PropertyMap& parameters, Log::LoggerPtr log, IDisposableParent* owner);

[[nodiscard]] IService* createSystemInfoService(Log::LoggerPtr log, IDisposableParent* owner);

} // namespace Er::Ipc::Grpc {}