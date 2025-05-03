#pragma once

#include <erebus/iunknown.hxx>

namespace grpc
{

class Service;

} // namespace grpc {}


namespace Er::Ipc::Grpc
{

struct IService
    : public IReferenceCounted
{
    static constexpr std::string_view IID = "Er.Ipc.Grpc.IService";

    virtual ::grpc::Service* grpc() noexcept = 0;
    virtual std::string_view name() const noexcept = 0;

protected:
    virtual ~IService() = default;
};


using ServicePtr = ReferenceCountedPtr<IService>;


} // namespace Er::Ipc::Grpc {}