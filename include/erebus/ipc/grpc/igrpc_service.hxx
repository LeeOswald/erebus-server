#pragma once

#include <erebus/iunknown.hxx>

namespace grpc
{

class Service;

} // namespace grpc {}


namespace Er::Ipc::Grpc
{

struct IService
    : public Er::IUnknown
{
    static constexpr std::string_view IID = "Er.Ipc.Grpc.IService";

    using Ptr = std::shared_ptr<IService>;

    virtual ~IService() = default;
    virtual ::grpc::Service* grpc() noexcept = 0;
    virtual std::string_view name() const noexcept = 0;
};

} // namespace Er::Ipc::Grpc {}