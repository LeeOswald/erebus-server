#include <erebus/knownprops.hxx>
#include <erebus-clt/erebus-clt.hxx>

#include <grpcpp/grpcpp.h>

#include <protocol/erebus.grpc.pb.h>

#include <atomic>

namespace Er
{

namespace Client
{

namespace
{

ResultCode mapGrpcStatus(grpc::StatusCode status)
{
    switch (status)
    {
    case grpc::OK: return ResultCode::Success;
    case grpc::CANCELLED: return ResultCode::Cancelled;
    case grpc::UNKNOWN: return ResultCode::Failure;
    case grpc::INVALID_ARGUMENT: return ResultCode::InvalidArgument;
    case grpc::DEADLINE_EXCEEDED: return ResultCode::DeadlineExceeded;
    case grpc::NOT_FOUND: return ResultCode::NotFound;
    case grpc::ALREADY_EXISTS: return ResultCode::AlreadyExists;
    case grpc::PERMISSION_DENIED: return ResultCode::PermissionDenied;
    case grpc::UNAUTHENTICATED: return ResultCode::Unauthenticated;
    case grpc::RESOURCE_EXHAUSTED: return ResultCode::ResourceExhausted;
    case grpc::FAILED_PRECONDITION: return ResultCode::FailedPrecondition;
    case grpc::ABORTED: return ResultCode::Aborted;
    case grpc::OUT_OF_RANGE: return ResultCode::OutOfRange;
    case grpc::UNIMPLEMENTED: return ResultCode::Unimplemented;
    case grpc::INTERNAL: return ResultCode::Internal;
    case grpc::UNAVAILABLE: return ResultCode::Unavailable;
    case grpc::DATA_LOSS: return ResultCode::DataLoss;
    default: return ResultCode::Failure;
    }
}

#define  throwGrpcStatusIfFailed(status) \
    if (!status.ok()) { \
        auto mappedStatus = mapGrpcStatus(status.error_code()); \
        auto message = status.error_message(); \
        throw Er::Exception(ER_HERE(), "RPC call failed", ::Er::Client::Props::ResultCode(mappedStatus), ::Er::ExceptionProps::DecodedError(std::move(message))); \
    } \



class Stub final
    : public Er::Client::IStub
    , public boost::noncopyable
{
public:
    explicit Stub(std::shared_ptr<grpc::Channel> channel)
        : m_stub(erebus::Erebus::NewStub(channel))
    {
    }

    void exit(bool restart) override
    {
        erebus::ExitRequest request;
        request.set_restart(restart);

        erebus::GenericReply reply;
        grpc::ClientContext context;
        grpc::Status status = m_stub->Exit(&context, request, &reply);
        throwGrpcStatusIfFailed(status);
    }

    Version version() override
    {
        erebus::Void request;

        erebus::ServerVersionReply reply;
        grpc::ClientContext context;
        grpc::Status status = m_stub->Version(&context, request, &reply);
        throwGrpcStatusIfFailed(status);

        return Version(reply.major(), reply.minor(), reply.patch());
    }

private:
    std::unique_ptr<erebus::Erebus::Stub> m_stub;
};

} // namespace {}


static std::atomic<long> g_initialized = 0;

EREBUSCLT_EXPORT void initialize()
{
    if (g_initialized.fetch_add(1, std::memory_order_acq_rel) == 0)
    {
        registerProperty(std::make_shared<PropertyInfoWrapper<::Er::Client::Props::ResultCode>>());

        ::grpc_init();
    }
}

EREBUSCLT_EXPORT void finalize()
{
    if (g_initialized.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        ::grpc_shutdown();

        unregisterProperty(lookupProperty(ER_PROPID_("erebus.ResultCode")));
    }
}

EREBUSCLT_EXPORT std::shared_ptr<IStub> create(const std::string& address)
{
    auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    return std::make_shared<Stub>(channel);
}

} // namespace Client {}

} // namespace Er {}