#pragma once

#include <erebus/ipc/grpc/client/iclient.hxx>
#include <erebus/rtl/log.hxx>
#include <erebus/rtl/time.hxx>


namespace Er::Ipc::Grpc
{

struct ISystemInfoClient
    : public IClient
{
    static constexpr std::string_view IID = "Er.Ipc.Grpc.ISystemInfoClient";

    struct PingMessage
    {
        Er::Time timestamp;
        std::uint64_t sequence;
        Er::Binary payload;
    };

    struct IPingCompletion
        : public IClient::ICompletion
    {
        virtual void onReply(PingMessage&& ping, PingMessage&& reply) = 0;

    protected:
        virtual ~IPingCompletion() = default;
    };

    struct ISystemInfoCompletion
        : public IClient::ICompletion
    {
        virtual CallbackResult onProperty(Property&& prop) = 0;

    protected:
        virtual ~ISystemInfoCompletion() = default;
    };
    
    virtual void ping(PingMessage&& ping, ReferenceCountedPtr<IPingCompletion> handler) = 0;
    virtual void getSystemInfo(const std::string& pattern, ReferenceCountedPtr<ISystemInfoCompletion> handler) = 0;

protected:
    virtual ~ISystemInfoClient() = default;
};


using SystemInfoClientPtr = ReferenceCountedPtr<ISystemInfoClient>;


} // namespace Er::Ipc::Grpc {}