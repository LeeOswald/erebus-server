#include <erebus/knownprops.hxx>
#include <erebus/util/format.hxx>
#include <erebus/util/random.hxx>
#include <erebus/util/sha256.hxx>
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

class MetadataCredentialsPlugin final
    : public grpc::MetadataCredentialsPlugin
{
public:
    MetadataCredentialsPlugin(const grpc::string_ref& metadataKey, const grpc::string_ref& metadataValue)
        : m_metadataKey(metadataKey.data(), metadataKey.length())
        , m_metadataValue(metadataValue.data(), metadataValue.length())
    {}

    grpc::Status GetMetadata(grpc::string_ref serviceUrl, grpc::string_ref methodName, const grpc::AuthContext& channelAuthContext, std::multimap<std::string, std::string>* metadata) override
    {
        metadata->insert(std::make_pair(m_metadataKey, m_metadataValue));
        return grpc::Status::OK;
    }

    std::string DebugString() override
    {
        return Er::Util::format("MetadataCredentials{key:%s,value:%s}", m_metadataKey.c_str(), m_metadataValue.c_str());
    }

private:
    std::string m_metadataKey;
    std::string m_metadataValue;
};

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

std::string messageFromResultCode(ResultCode code)
{
    switch (code)
    {
    case ResultCode::Success: return std::string("Success");
    case ResultCode::Failure: return std::string("Failure");
    case ResultCode::Cancelled: return std::string("Cancelled");
    case ResultCode::InvalidArgument: return std::string("Invalid argument");
    case ResultCode::DeadlineExceeded: return std::string("Deadline exceeded");
    case ResultCode::NotFound: return std::string("Not found");
    case ResultCode::AlreadyExists: return std::string("Already exists");
    case ResultCode::PermissionDenied: return std::string("Permission denied");
    case ResultCode::Unauthenticated: return std::string("Unauthenticated");
    case ResultCode::ResourceExhausted: return std::string("Resource exhausted");
    case ResultCode::FailedPrecondition: return std::string("Failed precondition");
    case ResultCode::Aborted: return std::string("Aborted");
    case ResultCode::OutOfRange: return std::string("Out of range");
    case ResultCode::Unimplemented: return std::string("Unimplemented");
    case ResultCode::Internal: return std::string("Internal");
    case ResultCode::Unavailable: return std::string("Unavailable");
    case ResultCode::DataLoss: return std::string("Data loss");
    default: return std::string("Unknown error");
    }
}

inline void throwGrpcStatusIfFailed(grpc::Status status)
{
    if (!status.ok())
    {
        auto mappedStatus = mapGrpcStatus(status.error_code());
        auto message = status.error_message();
        throw Er::Exception(ER_HERE(), "RPC call failed", ::Er::Client::Props::ResultCode(mappedStatus), ::Er::ExceptionProps::DecodedError(std::move(message)));
    }
}

inline void throwResultCodeIfFailed(erebus::ResultCode code)
{
    if (ResultCode(code) != ResultCode::Success) 
    {
        auto message = messageFromResultCode(ResultCode(code));
        throw Er::Exception(ER_HERE(), "RPC call failed", ::Er::Client::Props::ResultCode(ResultCode(code)), ::Er::ExceptionProps::DecodedError(std::move(message)));
    }
}

class Stub final
    : public Er::Client::IStub
    , public boost::noncopyable
{
public:
    explicit Stub(std::shared_ptr<grpc::Channel> channel, const Params& params)
        : m_stub(erebus::Erebus::NewStub(channel))
        , m_params(params)
    {
        checkAuth();
    }

    void addUser(std::string_view name, std::string_view password)
    {
        auto salt = makeSalt();
        Er::Util::Sha256 sha;
        sha.update(salt);
        sha.update(password);

        erebus::AddUserRequest request;
        request.set_name(std::string(name));
        request.set_salt(salt);
        request.set_pwd(sha.str(sha.digest()));

        erebus::GenericReply reply;
        grpc::ClientContext context;
        makeClientContext(context);

        grpc::Status status = m_stub->AddUser(&context, request, &reply);
        throwGrpcStatusIfFailed(status);
        throwResultCodeIfFailed(reply.code());
    }

    void exit(bool restart) override
    {
        erebus::ExitRequest request;
        request.set_restart(restart);

        erebus::GenericReply reply;
        grpc::ClientContext context;
        makeClientContext(context);
        
        grpc::Status status = m_stub->Exit(&context, request, &reply);
        throwGrpcStatusIfFailed(status);
        throwResultCodeIfFailed(reply.code());
    }

    Version version() override
    {
        erebus::Void request;

        erebus::ServerVersionReply reply;
        grpc::ClientContext context;
        makeClientContext(context);

        grpc::Status status = m_stub->Version(&context, request, &reply);
        throwGrpcStatusIfFailed(status);
        throwResultCodeIfFailed(reply.header().code());

        return Version(reply.major(), reply.minor(), reply.patch());
    }

private:
    void checkAuth()
    {
        if (!m_params.ssl || m_autorized)
            return;

        // request salt
        std::string salt;
        {
            erebus::InitialRequest request;
            request.set_user(m_params.user);

            erebus::InitialReply reply;
            grpc::ClientContext context;

            grpc::Status status = m_stub->Init(&context, request, &reply);
            throwGrpcStatusIfFailed(status);
            throwResultCodeIfFailed(reply.header().code());
            salt = reply.salt();
        }

        // request ticket
        {
            Er::Util::Sha256 sha;
            sha.update(salt);
            sha.update(m_params.password);
            auto hash = sha.digest();
            auto hashStr = sha.str(hash);

            erebus::AuthRequest request;
            request.set_user(m_params.user);
            request.set_pwd(hashStr);

            erebus::AuthReply reply;
            grpc::ClientContext context;

            grpc::Status status = m_stub->Authorize(&context, request, &reply);
            throwGrpcStatusIfFailed(status);
            throwResultCodeIfFailed(reply.header().code());

            m_ticket = reply.ticket();
            m_autorized = true;
        }
    }

    void makeClientContext(grpc::ClientContext& context)
    {
        if (m_autorized)
        {
            assert(!m_ticket.empty());
            auto creds = grpc::MetadataCredentialsFromPlugin(std::unique_ptr<grpc::MetadataCredentialsPlugin>(new MetadataCredentialsPlugin("ticket", m_ticket)));
            context.set_credentials(creds);
        }
    }

    std::string makeSalt()
    {
        Er::Util::Random r;
        return r.generate(kSaltLength, kSaltChars);
    }

    const size_t kSaltLength = 8;
    const std::string_view kSaltChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";

    std::unique_ptr<erebus::Erebus::Stub> m_stub;
    Params m_params;
    bool m_autorized = false;
    std::string m_ticket;
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

EREBUSCLT_EXPORT std::shared_ptr<IStub> create(const Params& params)
{
    bool local = params.endpoint.starts_with("unix:");
    if (!local && params.ssl)
    {
        grpc::SslCredentialsOptions opts;
        opts.pem_root_certs = params.rootCA;

        auto channelCreds = grpc::SslCredentials(opts);
        auto channel = grpc::CreateChannel(params.endpoint, channelCreds);
        return std::make_shared<Stub>(channel, params);
    }
    else
    {
        auto channel = grpc::CreateChannel(params.endpoint, grpc::InsecureChannelCredentials());
        return std::make_shared<Stub>(channel, params);
    }
}

} // namespace Client {}

} // namespace Er {}