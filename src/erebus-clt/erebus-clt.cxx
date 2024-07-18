#include <erebus/erebus.grpc.pb.h>
#include <grpcpp/grpcpp.h>

#include <erebus/knownprops.hxx>
#include <erebus/protocol.hxx>
#include <erebus/result.hxx>
#include <erebus/util/format.hxx>
#include <erebus-clt/erebus-clt.hxx>

#include <atomic>
#include <sstream>

namespace Er
{

namespace Client
{

namespace
{


class ClientImpl final
    : public Er::Client::IClient
    , public Er::NonCopyable
{
public:
    ~ClientImpl()
    {
    }

    explicit ClientImpl(std::shared_ptr<grpc::Channel> channel, Er::Log::ILog* log)
        : m_stub(erebus::Erebus::NewStub(channel))
        , m_log(log)
    {
    }

    SessionId beginSession(std::string_view req) override
    {
        erebus::AllocateSessionRequest request;
        request.set_request(std::string(req));

        erebus::AllocateSessionReply reply;
        grpc::ClientContext context;
        grpc::Status status = m_stub->AllocateSession(&context, request, &reply);
        throwIfFailed(status, &reply.header());

        return reply.sessionid();
    }

    void endSession(std::string_view req, SessionId id) override
    {
        erebus::DeleteSessionRequest request;
        request.set_request(std::string(req));

        erebus::GenericReply reply;
        grpc::ClientContext context;
        grpc::Status status = m_stub->DeleteSession(&context, request, &reply);
        throwIfFailed(status, &reply);
    }

    Er::PropertyBag request(std::string_view req, const Er::PropertyBag& args, std::optional<SessionId> sessionId) override
    {
        erebus::ServiceRequest request;
        request.set_request(std::string(req));
        if (sessionId)
            request.set_sessionid(*sessionId);

        // marshal properties
        Er::enumerateProperties(args, [&request](const Property& arg)
        {
            auto a = request.add_args();
            Er::Protocol::assignProperty(*a, arg);
        });

        erebus::ServiceReply reply;
        grpc::ClientContext context;
        grpc::Status status = m_stub->GenericRpc(&context, request, &reply);
        throwIfFailed(status, &reply.header());

        // unmarshal properties
        Er::PropertyBag bag;
        int count = reply.props_size();
        for (int i = 0; i < count; ++i)
        {
            auto& prop = reply.props(i);
            Er::insertProperty(bag, Er::Protocol::getProperty(prop));
        }

        return bag;
    }

    std::vector<Er::PropertyBag> requestStream(std::string_view req, const Er::PropertyBag& args, std::optional<SessionId> sessionId) override
    {
        erebus::ServiceRequest request;
        request.set_request(std::string(req));
        if (sessionId)
            request.set_sessionid(*sessionId);

        // marshal properties
        Er::enumerateProperties(args, [&request](const Property& arg)
        {
            auto a = request.add_args();
            Er::Protocol::assignProperty(*a, arg);
        });

        grpc::ClientContext context;
        std::shared_ptr<grpc::ClientReader<erebus::ServiceReply>> stream(m_stub->GenericStream(&context, request));
        
        std::vector<Er::PropertyBag> out;
        erebus::ServiceReply reply;
        while (stream->Read(&reply))
        {
            throwIfFailed(grpc::Status::OK, &reply.header());

            // unmarshal properties
            Er::PropertyBag bag;
            int count = reply.props_size();
            for (int i = 0; i < count; ++i)
            {
                auto& prop = reply.props(i);
                Er::insertProperty(bag, Er::Protocol::getProperty(prop));
            }

            if (!bag.empty())
                out.push_back(std::move(bag));
        }

        return out;
    }

private:
    static Result mapGrpcStatus(grpc::StatusCode status) noexcept
    {
        switch (status)
        {
        case grpc::OK: return Result::Success;
        case grpc::CANCELLED: return Result::Cancelled;
        case grpc::UNKNOWN: return Result::Failure;
        case grpc::INVALID_ARGUMENT: return Result::InvalidArgument;
        case grpc::DEADLINE_EXCEEDED: return Result::DeadlineExceeded;
        case grpc::NOT_FOUND: return Result::NotFound;
        case grpc::ALREADY_EXISTS: return Result::AlreadyExists;
        case grpc::PERMISSION_DENIED: return Result::PermissionDenied;
        case grpc::UNAUTHENTICATED: return Result::Unauthenticated;
        case grpc::RESOURCE_EXHAUSTED: return Result::ResourceExhausted;
        case grpc::FAILED_PRECONDITION: return Result::FailedPrecondition;
        case grpc::ABORTED: return Result::Aborted;
        case grpc::OUT_OF_RANGE: return Result::OutOfRange;
        case grpc::UNIMPLEMENTED: return Result::Unimplemented;
        case grpc::INTERNAL: return Result::Internal;
        case grpc::UNAVAILABLE: return Result::Unavailable;
        case grpc::DATA_LOSS: return Result::DataLoss;
        default: return Result::Failure;
        }
    }

    void throwIfFailed(grpc::Status status, const erebus::GenericReply* reply)
    {
        if (!status.ok())
        {
            auto mappedStatus = mapGrpcStatus(status.error_code());
            throw Er::Exception(ER_HERE(), "RPC call failed", ::Er::ExceptionProps::ResultCode(static_cast<int32_t>(mappedStatus)), ::Er::ExceptionProps::DecodedError(status.error_message()));
        }

        if (!reply)
            return;

        if (reply->has_exception())
        {
            // unmarshal and throw the exception
            auto& exception = reply->exception();
            std::string_view message;
            if (exception.has_message())
                message = exception.message();
            else
                message = "Unknown exception";

            Er::Exception unmarshaledException(ER_HERE(), std::move(message));

            auto propCount = exception.props_size();
            
            for (int i = 0; i < propCount; ++i)
            {
                auto& prop = exception.props(i);
                unmarshaledException.add(Er::Protocol::getProperty(prop));
            }
            
            throw unmarshaledException;
        }
    }

    std::unique_ptr<erebus::Erebus::Stub> m_stub;
    Er::Log::ILog* const m_log;
};


LibParams g_libParams;
std::atomic<long> g_initialized = 0;

void gprLogFunction(gpr_log_func_args* args)
{
    if (g_libParams.log)
    {
        Er::Log::Level level = Er::Log::Level::Debug;
        switch (args->severity)
        {
        case GPR_LOG_SEVERITY_INFO: level = Log::Level::Info; break;
        case GPR_LOG_SEVERITY_ERROR: level = Log::Level::Error; break;
        }

        g_libParams.log->writef(level, "[gRPC] %s", args->message);
    }
}

} // namespace {}


EREBUSCLT_EXPORT void initialize(const LibParams& params)
{
    if (g_initialized.fetch_add(1, std::memory_order_acq_rel) == 0)
    {
        g_libParams = params;
        Er::Protocol::Props::Private::registerAll(params.log);
        
        ::grpc_init();

        if (params.level == Log::Level::Debug)
            ::gpr_set_log_verbosity(GPR_LOG_SEVERITY_DEBUG);
        else
            ::gpr_set_log_verbosity(GPR_LOG_SEVERITY_INFO);

        ::gpr_set_log_function(gprLogFunction);

    }
}

EREBUSCLT_EXPORT void finalize()
{
    if (g_initialized.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        ::grpc_shutdown();
        
        Er::Protocol::Props::Private::unregisterAll(g_libParams.log);
        g_libParams = LibParams();
    }
}

EREBUSCLT_EXPORT std::shared_ptr<void> createChannel(const ChannelParams& params)
{
    bool local = params.endpoint.starts_with("unix:");

    grpc::ChannelArguments args;
#if !ER_DEBUG
    if (!local)
    {
        args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 20 * 1000);
        args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 10 * 1000);
        args.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
    }
#endif

    if (!local && params.ssl)
    {
        grpc::SslCredentialsOptions opts;
        opts.pem_root_certs = params.rootCertificate;
        opts.pem_cert_chain = params.certificate;
        opts.pem_private_key = params.key;

        auto channelCreds = grpc::SslCredentials(opts);
        return grpc::CreateCustomChannel(params.endpoint, channelCreds, args);
    }
    else
    {
        return grpc::CreateCustomChannel(params.endpoint, grpc::InsecureChannelCredentials(), args);
    }
}

EREBUSCLT_EXPORT std::shared_ptr<IClient> createClient(std::shared_ptr<void> channel, Log::ILog* log)
{
    return std::make_shared<ClientImpl>(std::static_pointer_cast<grpc::Channel>(channel), log);
}

} // namespace Client {}

} // namespace Er {}