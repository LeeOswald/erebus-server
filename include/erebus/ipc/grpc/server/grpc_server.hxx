#pragma once

#include <erebus/ipc/grpc/server/iserver.hxx>
#include <erebus/rtl/log.hxx>
#include <erebus/rtl/property_bag.hxx>


namespace Er::Ipc::Grpc
{

[[nodiscard]] ServerPtr createServer(const PropertyMap& parameters, Log::LoggerPtr log);

[[nodiscard]] ServicePtr createSystemInfoService(Log::LoggerPtr log);

} // namespace Er::Ipc::Grpc {}