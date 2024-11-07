#include <erebus/erebus.grpc.pb.h>
#include <grpcpp/grpcpp.h>

#include <erebus/knownprops.hxx>
#include <erebus/protocol.hxx>
#include <erebus/result.hxx>
#include <erebus-clt/erebus-clt.hxx>
#include <erebus-srv/global_requests.hxx>

#include <atomic>
#include <random>
#include <sstream>

namespace Er::Client
{

namespace
{

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
        , m_cookie(makeCookie())
    {
    }

    void request(CallId callId, std::string_view request, const Er::PropertyBag& args, IReceiver* receiver, std::optional<std::chrono::milliseconds> timeout) override
    {
        Er::Log::debug(m_log, "{}.ClientReadReactor::request({}.{})", Er::Format::ptr(this), callId, request);
        Er::Log::Indent idt(m_log);

        auto ctx = std::make_shared<UnaryCallbackContext>(callId, request, args, m_cookie, receiver);
        if (timeout)
        {
            ctx->context.set_deadline(std::chrono::system_clock::now() + *timeout);
        }

        m_stub->async()->GenericRpc(
            &ctx->context,
            &ctx->request,
            &ctx->reply,
            [ctx](grpc::Status status)
            {
                if (!status.ok())
                {
                    ctx->receiver->receive(ctx->callId, mapGrpcStatus(status.error_code()), status.error_message());
                }
                else if (ctx->reply.has_exception())
                {
                    ctx->receiver->receive(ctx->callId, unmarshalException(ctx->reply));
                }
                else
                {
                    ctx->receiver->receive(ctx->callId, unmarshal(ctx->reply));
                }
            });
    }

    void requestStream(CallId callId, std::string_view request, const Er::PropertyBag& args, IStreamReceiver* receiver) override
    {
        Er::Log::debug(m_log, "{}.ClientReadReactor::requestStream({}.{})", Er::Format::ptr(this), callId, request);
        Er::Log::Indent idt(m_log);

        new ClientReadReactor(m_stub.get(), m_log, callId, request, args, m_cookie, receiver);
    }

private:
    struct CallContext
        : public Er::NonCopyable
    {
        virtual ~CallContext() = default;

        erebus::ServiceRequest request;
        grpc::ClientContext context;

        CallContext(std::string_view req, const Er::PropertyBag& args, const std::string& cookie)
        {
            request.set_request(std::string(req));
            request.set_cookie(cookie);

            Er::enumerateProperties(args, [this](const Property& arg)
            {
                auto a = request.add_args();
                Er::Protocol::assignProperty(*a, arg);
            });
        }
    };

    struct UnaryCallbackContext final
        : public CallContext
    {
        CallId callId;
        erebus::ServiceReply reply;
        IReceiver* receiver;

        UnaryCallbackContext(CallId callId, std::string_view req, const Er::PropertyBag& args, const std::string& cookie, IReceiver* receiver)
            : CallContext(req, args, cookie)
            , callId(callId)
            , receiver(receiver)
        {
        }
    };

    class ClientReadReactor final
        : public grpc::ClientReadReactor<erebus::ServiceReply>
    {
    public:
        ~ClientReadReactor()
        {
            Er::Log::debug(m_log, "{}.ClientReadReactor::~ClientReadReactor({})", Er::Format::ptr(this), m_callId);
            Er::Log::Indent idt(m_log);
        }

        ClientReadReactor(erebus::Erebus::Stub* stub, Er::Log::ILog* log, CallId callId, std::string_view req, const Er::PropertyBag& args, const std::string& cookie, IStreamReceiver* receiver)
            : m_log(log)
            , m_context(req, args, cookie)
            , m_callId(callId)
            , m_receiver(receiver)
        {
            Er::Log::debug(m_log, "{}.ClientReadReactor::ClientReadReactor({})", Er::Format::ptr(this), m_callId);
            Er::Log::Indent idt(m_log);

            stub->async()->GenericStream(&m_context.context, &m_context.request, this);
            StartRead(&m_reply);
            StartCall();
        }

        void OnReadDone(bool ok) override 
        {
            Er::Log::debug(m_log, "{}.ClientReadReactor::OnReadDone({}, {})", Er::Format::ptr(this), m_callId, ok);
            Er::Log::Indent idt(m_log);

            if (ok) 
            {
                if (m_reply.has_exception())
                {
                    if (m_receiver->receive(m_callId, unmarshalException(m_reply)) == IClient::IStreamReceiver::Result::Cancel)
                    {
                        m_context.context.TryCancel();
                    }
                }
                else if (m_receiver->receive(m_callId, unmarshal(m_reply)) == IClient::IStreamReceiver::Result::Cancel)
                {
                    m_context.context.TryCancel();
                }
                
                // we have to drain the completion queue even if we cancel
                StartRead(&m_reply);
            }
        }

        void OnDone(const grpc::Status& status) override
        {
            Er::Log::debug(m_log, "{}.ClientReadReactor::OnDone({}, {})", Er::Format::ptr(this), m_callId, int(status.error_code()));
            Er::Log::Indent idt(m_log);

            if (!status.ok())
            {
                m_receiver->finish(m_callId, mapGrpcStatus(status.error_code()), status.error_message());
            }
            else
            {
                m_receiver->finish(m_callId);
            }

            delete this;
        }
  
    private:
        Er::Log::ILog* m_log;
        CallContext m_context;
        CallId m_callId;
        erebus::ServiceReply m_reply;
        IStreamReceiver* m_receiver;
    };

    static Er::Exception unmarshalException(const erebus::ServiceReply& reply)
    {
        ErAssert(reply.has_exception());
        
        auto& exception = reply.exception();
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

        return unmarshaledException;
    }

    static Er::PropertyBag unmarshal(const erebus::ServiceReply& reply)
    {
        Er::PropertyBag bag;
        int count = reply.props_size();
        for (int i = 0; i < count; ++i)
        {
            auto& prop = reply.props(i);
            Er::addProperty(bag, Er::Protocol::getProperty(prop));
        }

        return bag;
    }

    static std::string makeCookie()
    {
        static const char ValidChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
        static const size_t ValidCharsCount = sizeof(ValidChars) - 1;
        static const size_t CookieLength = 32;

        std::uniform_int_distribution<> charDistrib(0, ValidCharsCount - 1);

        static std::random_device rd;
        std::mt19937 random(rd());

        std::string cookie(CookieLength, ' ');
        for (size_t i = 0; i < CookieLength; ++i)
            cookie[i] = ValidChars[charDistrib(random)];

        return cookie;
    }

    std::unique_ptr<erebus::Erebus::Stub> m_stub;
    Er::Log::ILog* const m_log;
    std::string m_cookie;
};


Er::Log::ILog* g_log = nullptr;
std::atomic<long> g_initialized = 0;

void gprLogFunction(gpr_log_func_args* args)
{
    if (g_log)
    {
        Er::Log::Level level = Er::Log::Level::Debug;
        switch (args->severity)
        {
        case GPR_LOG_SEVERITY_INFO: level = Log::Level::Info; break;
        case GPR_LOG_SEVERITY_ERROR: level = Log::Level::Error; break;
        }

        Er::Log::write(g_log, level, "[gRPC] {}", args->message);
    }
}

} // namespace {}


EREBUSCLT_EXPORT void initialize(Er::Log::ILog* log)
{
    if (g_initialized.fetch_add(1, std::memory_order_acq_rel) == 0)
    {
        g_log = log;
        Er::Server::Props::Private::registerAll(g_log);
        
        ::grpc_init();

        if (log->level() == Log::Level::Debug)
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
        
        Er::Server::Props::Private::unregisterAll(g_log);
        g_log = nullptr;
    }
}

EREBUSCLT_EXPORT ChannelPtr createChannel(const ChannelParams& params)
{
    grpc::ChannelArguments args;

    if (params.keepAlive)
    {
        args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 20 * 1000);
        args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 10 * 1000);
        args.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
    }

    if (params.ssl)
    {
        grpc::SslCredentialsOptions opts;
        opts.pem_root_certs = params.rootCa;
        opts.pem_cert_chain = params.certificate;
        opts.pem_private_key = params.privateKey;

        auto channelCreds = grpc::SslCredentials(opts);
        return grpc::CreateCustomChannel(params.endpoint, channelCreds, args);
    }
    else
    {
        return grpc::CreateCustomChannel(params.endpoint, grpc::InsecureChannelCredentials(), args);
    }
}

EREBUSCLT_EXPORT IClient::Ptr createClient(ChannelPtr channel, Log::ILog* log)
{
    return std::make_unique<ClientImpl>(std::static_pointer_cast<grpc::Channel>(channel), log);
}

} // namespace Er::Client {}