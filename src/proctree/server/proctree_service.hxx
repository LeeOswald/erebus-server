#pragma once

#include <erebus/ipc/grpc/server/iservice.hxx>
#include <erebus/rtl/log.hxx>


namespace Er::ProcessTree::Private
{

Er::Ipc::Grpc::ServicePtr createProcessListService(Er::Log::ILogger* log);


} // namespace Er::ProcessTree::Private {}
