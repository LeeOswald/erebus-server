#pragma once

#include <erebus/iunknown.hxx>
#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/log.hxx>
#include <erebus/rtl/property_bag.hxx>

namespace grpc
{

class Channel;

} // namespace grpc {}

namespace Er::Ipc::Grpc
{

using ChannelPtr = std::shared_ptr<grpc::Channel>;

[[nodiscard]] ChannelPtr createChannel(const PropertyMap& parameters);


struct IClient
    : public IUnknown
{
    static constexpr std::string_view IID = "Er.Ipc.Grpc.IClient";

    using Ptr = std::shared_ptr<IClient>;

    virtual ~IClient() = default;

    struct ICompletion
    {
        using Ptr = std::shared_ptr<ICompletion>;

        virtual ~ICompletion() = default;

        virtual void onError(Er::ResultCode result, std::string&& message) noexcept = 0;
    };
};

} // namespace Er::Ipc::Grpc {}