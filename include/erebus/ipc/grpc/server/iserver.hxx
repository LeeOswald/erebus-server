#pragma once

#include <erebus/ipc/grpc/server/iservice.hxx>

namespace grpc
{

class Server;

} // namespace grpc {}


namespace Er::Ipc::Grpc
{

struct IServer
    : public IReferenceCounted
{
    static constexpr std::string_view IID = "Er.Ipc.Grpc.IServer";

    virtual ::grpc::Server* grpc() noexcept = 0;
    virtual void addService(ServicePtr service) = 0;
    virtual void start() = 0;

protected:
    virtual ~IServer() = default;
};


using ServerPtr = ReferenceCountedPtr<IServer>;


} // namespace Er::Ipc::Grpc {}