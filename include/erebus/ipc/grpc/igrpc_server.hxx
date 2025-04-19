#pragma once

#include <erebus/ipc/grpc/igrpc_service.hxx>

namespace grpc
{

class Server;

} // namespace grpc {}


namespace Er::Ipc::Grpc
{

struct IServer
    : public Er::IUnknown
{
    static constexpr std::string_view IID = "Er.Ipc.Grpc.IServer";

    using Ptr = std::shared_ptr<IServer>;

    virtual ~IServer() = default;
    virtual ::grpc::Server* grpc() noexcept = 0;
    virtual void addService(IService::Ptr service) = 0;
    virtual void start() = 0;
};


} // namespace Er::Ipc::Grpc {}