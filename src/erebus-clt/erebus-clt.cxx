#include <erebus/knownprops.hxx>
#include <erebus/util/format.hxx>
#include <erebus/util/random.hxx>
#include <erebus/util/sha256.hxx>
#include <erebus-clt/erebus-clt.hxx>

#include <grpcpp/grpcpp.h>

#include <erebus/erebus.grpc.pb.h>

#include <atomic>
#include <sstream>

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


class Stub final
    : public Er::Client::IStub
    , public boost::noncopyable
{
public:
    ~Stub()
    {
        disconnect();
    }

    explicit Stub(std::shared_ptr<grpc::Channel> channel, const Params& params)
        : m_stub(erebus::Erebus::NewStub(channel))
        , m_params(params)
    {
        checkAuth();
    }

    void addUser(std::string_view name, std::string_view password) override
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
        throwIfFailed(status, &reply);
    }

    void removeUser(std::string_view name) override
    {
        erebus::RemoveUserRequest request;
        request.set_name(std::string(name));

        erebus::GenericReply reply;
        grpc::ClientContext context;
        makeClientContext(context);

        grpc::Status status = m_stub->RemoveUser(&context, request, &reply);
        throwIfFailed(status, &reply);
    }

    std::vector<UserInfo> listUsers() override
    {
        erebus::Void request;

        erebus::ListUsersReply reply;
        grpc::ClientContext context;
        makeClientContext(context);

        grpc::Status status = m_stub->ListUsers(&context, request, &reply);
        throwIfFailed(status, &reply.header());

        std::vector<UserInfo> v;
        auto userCount = reply.users_size();
        if (userCount)
        {
            v.reserve(userCount);
            for (int i = 0; i < userCount; ++i)
            {
                auto& u = reply.users(i);
                v.emplace_back(u.name());
            }
        }

        return v;
    }

    void exit(bool restart) override
    {
        erebus::ExitRequest request;
        request.set_restart(restart);

        erebus::GenericReply reply;
        grpc::ClientContext context;
        makeClientContext(context);
        
        grpc::Status status = m_stub->Exit(&context, request, &reply);
        throwIfFailed(status, &reply);
    }

    Version version() override
    {
        erebus::Void request;

        erebus::ServerVersionReply reply;
        grpc::ClientContext context;
        makeClientContext(context);

        grpc::Status status = m_stub->Version(&context, request, &reply);
        throwIfFailed(status, &reply.header());

        return Version(reply.major(), reply.minor(), reply.patch());
    }

private:
    void disconnect()
    {
        if (!m_stub)
            return;

        erebus::Void request;

        erebus::Void reply;
        grpc::ClientContext context;
        makeClientContext(context);

        grpc::Status status = m_stub->Disconnect(&context, request, &reply);
    }

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
            throwIfFailed(status, &reply.header());
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
            throwIfFailed(status, &reply.header());

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

    static ResultCode mapGrpcStatus(grpc::StatusCode status) noexcept
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

    void throwIfFailed(grpc::Status status, const erebus::GenericReply* reply)
    {
        if (!status.ok())
        {
            auto mappedStatus = mapGrpcStatus(status.error_code());
            throw Er::Exception(ER_HERE(), "RPC call failed", ::Er::Client::Props::ResultCode(mappedStatus), ::Er::ExceptionProps::DecodedError(status.error_message()));
        }

        if (!reply)
            return;

        if (!reply->has_exception())
        {
            // no exceptions, check the returned error code
            auto code = ResultCode(reply->code());
            if (code != ResultCode::Success)
            {
                std::ostringstream ss;
                ss << code;
                
                throw Er::Exception(ER_HERE(), "RPC call failed", ::Er::Client::Props::ResultCode(code), ::Er::ExceptionProps::DecodedError(ss.str()));
            }
        }
        else
        {
            // unmarshal and throw the exception
            auto& exception = reply->exception();
            std::string_view message;
            if (exception.has_message())
                message = exception.message();
            else
                message = "Unknown exception";

            Er::Exception::Location location;
            if (exception.has_source())
            {
                auto& source = exception.source();
                location.source = Er::SourceLocation(source.file(), source.line());
            }

            if (exception.has_stack())
            {
                auto& stack = exception.stack();
                auto frameCount = stack.frames_size();
                if (frameCount)
                {
                    location.decoded = Er::DecodedStackTrace();
                    location.decoded->reserve(frameCount);

                    for (int i = 0; i < frameCount; ++i)
                    {
                        location.decoded->emplace_back(stack.frames(i));
                    }
                }
            }

            Er::Exception unmarshaledException(std::move(location), std::move(message));

            auto propCount = exception.props_size();
            if (propCount)
            {
                for (int i = 0; i < propCount; ++i)
                {
                    auto& prop = exception.props(i);
                    auto id = prop.id();
                    auto info = Er::lookupProperty(id);
                    if (!info)
                    {
                        m_params.log->write(Er::Log::Level::Error, "Unsupported exception property 0x%08x", id);
                    }
                    else
                    {
                        auto& type = info->type();
                        if (type == typeid(bool))
                            unmarshaledException.add(id, prop.v_bool());
                        else if (type == typeid(int32_t))
                            unmarshaledException.add(id, prop.v_int32());
                        else if (type == typeid(uint32_t))
                            unmarshaledException.add(id, prop.v_uint32());
                        else if (type == typeid(int64_t))
                            unmarshaledException.add(id, prop.v_int64());
                        else if (type == typeid(uint64_t))
                            unmarshaledException.add(id, prop.v_uint64());
                        else if (type == typeid(double))
                            unmarshaledException.add(id, prop.v_double());
                        else if (type == typeid(std::string))
                            unmarshaledException.add(id, prop.v_string());
                        else
                            m_params.log->write(Er::Log::Level::Error, "Unsupported exception property type %s", type.name());
                    }
                }
            }

            throw unmarshaledException;
        }
    }

    const size_t kSaltLength = 8;
    const std::string_view kSaltChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";

    std::unique_ptr<erebus::Erebus::Stub> m_stub;
    Params m_params;
    bool m_autorized = false;
    std::string m_ticket;
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

        g_libParams.log->write(level, "[gRPC] %s", args->message);
    }
}

} // namespace {}


EREBUSCLT_EXPORT void initialize(const LibParams& params)
{
    if (g_initialized.fetch_add(1, std::memory_order_acq_rel) == 0)
    {
        g_libParams = params;
        
        registerProperty(std::make_shared<PropertyInfoWrapper<::Er::Client::Props::ResultCode>>());

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
        g_libParams = LibParams();

        unregisterProperty(lookupProperty(ER_PROPID_("erebus.ResultCode")));
    }
}

EREBUSCLT_EXPORT std::shared_ptr<IStub> create(const Params& params)
{
    bool local = params.endpoint.starts_with("unix:");
    
    grpc::ChannelArguments args;
    args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 20 * 1000);
    args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 10 * 1000);
    args.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);

    if (!local && params.ssl)
    {
        grpc::SslCredentialsOptions opts;
        opts.pem_root_certs = params.rootCA;

        auto channelCreds = grpc::SslCredentials(opts);
        auto channel = grpc::CreateCustomChannel(params.endpoint, channelCreds, args);
        return std::make_shared<Stub>(channel, params);
    }
    else
    {
        auto channel = grpc::CreateCustomChannel(params.endpoint, grpc::InsecureChannelCredentials(), args);
        return std::make_shared<Stub>(channel, params);
    }
}

} // namespace Client {}

} // namespace Er {}