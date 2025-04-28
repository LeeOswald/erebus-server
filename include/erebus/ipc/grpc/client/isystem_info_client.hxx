#pragma once

#include <erebus/ipc/grpc/client/iclient.hxx>
#include <erebus/rtl/log.hxx>
#include <erebus/rtl/system/packed_time.hxx>


namespace Er::Ipc::Grpc
{

struct ISystemInfoClient
    : public IClient
{
    static constexpr std::string_view IID = "Er.Ipc.Grpc.ISystemInfoClient";

    using Ptr = std::shared_ptr<ISystemInfoClient>;

    struct PingMessage
    {
        Er::System::PackedTime timestamp;
        std::uint64_t sequence;
        Er::Binary payload;
    };

    struct IPingCompletion
        : public IClient::ICompletion
    {
        using Ptr = std::shared_ptr<IPingCompletion>;

        virtual ~IPingCompletion() = default;
        virtual void onReply(PingMessage&& ping, PingMessage&& reply) = 0;
    };

    struct ISystemInfoCompletion
        : public IClient::ICompletion
    {
        using Ptr = std::shared_ptr<ISystemInfoCompletion>;

        virtual ~ISystemInfoCompletion() = default;
        virtual CallbackResult onProperty(Property&& prop) = 0;
    };

    virtual ~ISystemInfoClient() = default;
    virtual void ping(PingMessage&& ping, IPingCompletion::Ptr handler) = 0;
    virtual void getSystemInfo(const std::string& pattern, ISystemInfoCompletion::Ptr handler) = 0;
};


} // namespace Er::Ipc::Grpc {}