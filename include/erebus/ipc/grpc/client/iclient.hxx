#pragma once

#include <erebus/iunknown.hxx>
#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/log.hxx>
#include <erebus/rtl/property_bag.hxx>


namespace grpc
{

class Channel;
class Status;


} // namespace grpc {}

namespace Er::Ipc::Grpc
{

using ChannelPtr = std::shared_ptr<grpc::Channel>;

[[nodiscard]] ChannelPtr createChannel(const PropertyMap& parameters);


struct IClient
    : public IReferenceCounted
{
    static constexpr std::string_view IID = "Er.Ipc.Grpc.IClient";

    struct ICompletion
        : public IReferenceCounted
    {
        virtual void onError(grpc::Status const& status) noexcept = 0;

    protected:
        virtual ~ICompletion() = default;
    };

protected:
    virtual ~IClient() = default;
};

} // namespace Er::Ipc::Grpc {}