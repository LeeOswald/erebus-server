#include <grpcpp/grpcpp.h>

#include <protobuf/system_info.grpc.pb.h>
#include <erebus/ipc/grpc/protocol.hxx>
#include <erebus/ipc/grpc/system_info_client.hxx>
#include <erebus/rtl/util/exception_util.hxx>

#include "trace.hxx"

#include <condition_variable>
#include <mutex>

#include <boost/noncopyable.hpp> 


namespace Er::Ipc::Grpc
{

class SystemInfoClientImpl
    : public ISystemInfoClient
    , public std::enable_shared_from_this<SystemInfoClientImpl>
{
public:
    ~SystemInfoClientImpl()
    {
        ClientTrace2(m_log.get(), "{}.SystemInfoClientImpl::~SystemInfoClientImpl", Er::Format::ptr(this));

        waitRunningContexts();

        ::grpc_shutdown();
    }

    SystemInfoClientImpl(ChannelPtr channel, Log::ILogger::Ptr log)
        : m_grpcReady(grpcInit())
        , m_stub(erebus::SystemInfo::NewStub(channel))
        , m_log(log)
    {
        ClientTrace2(m_log.get(), "{}.SystemInfoClientImpl::SystemInfoClientImpl", Er::Format::ptr(this));
    }

    IUnknown::Ptr queryInterface(std::string_view iid) noexcept override
    {
        if ((iid == IClient::IID) ||
            (iid == IUnknown::IID))
        {
            return shared_from_this();
        }

        return {};
    }

    void ping(PingMessage&& ping, IPingCompletion::Ptr handler) override
    {
        ClientTrace2(m_log.get(), "{}.SystemInfoClientImpl::ping", Er::Format::ptr(this));

        auto ctx = std::make_shared<PingContext>(this, m_log.get(), std::move(ping), handler);

        m_stub->async()->Ping(
            &ctx->grpcContext,
            &ctx->request,
            &ctx->reply,
            [this, ctx](grpc::Status status)
            {
                completePing(ctx, status);
            });
    }

private:
    struct ContextBase
        : public boost::noncopyable
    {
        ~ContextBase() noexcept
        {
            ClientTrace2(m_log, "{}.ContextBase::~ContextBase()", Er::Format::ptr(this));
            m_owner->removeContext();
        }

        ContextBase(SystemInfoClientImpl* owner, Er::Log::ILogger* log) noexcept
            : m_owner(owner)
            , m_log(log)
        {
            ClientTrace2(m_log, "{}.ContextBase::ContextBase()", Er::Format::ptr(this));
            owner->addContext();
        }

        grpc::ClientContext grpcContext;

    protected:
        SystemInfoClientImpl* const m_owner;
        Er::Log::ILogger* const m_log;
    };

    struct PingContext
        : public ContextBase
    {

        PingContext(SystemInfoClientImpl* owner, Er::Log::ILogger* log, PingMessage&& ping, IPingCompletion::Ptr handler)
            : ContextBase(owner, log)
            , handler(handler)
            , ping(std::move(ping))
        {
            request.set_timestamp(this->ping.timestamp.value);
            request.set_sequence(this->ping.sequence);
            request.set_payload(this->ping.payload.bytes());
        }

        IPingCompletion::Ptr handler;
        PingMessage ping;
        erebus::PingMessage request;
        erebus::PingMessage reply;
    };

    void completePing(std::shared_ptr<PingContext> ctx, grpc::Status status)
    {
        ClientTraceIndent2(m_log.get(), "{}.SystemInfoClientImpl::completePing", Er::Format::ptr(this));

        Er::Util::ExceptionLogger xcptLogger(m_log.get());

        try
        {
            if (!status.ok())
            {
                // transport failure or something
                auto resultCode = mapGrpcStatus(status.error_code());
                auto errorMsg = status.error_message();
                ErLogError2(m_log.get(), "Failed to ping {}: {} ({})", ctx->grpcContext.peer(), resultCode, errorMsg);

                ctx->handler->onError(resultCode, std::move(errorMsg));
            }
            else
            {
                ClientTrace2(m_log.get(), "Pinged {} with {} bytes of data in {} msec", ctx->grpcContext.peer(), ctx->ping.payload.size(), (ctx->reply.timestamp() - ctx->ping.timestamp.value) / 1000);

                PingMessage reply;
                reply.timestamp = ctx->reply.timestamp();
                reply.sequence = ctx->reply.sequence();
                reply.payload = Binary(ctx->reply.payload());

                ctx->handler->onReply(std::move(ctx->ping), std::move(reply));
            }
        }
        catch (...)
        {
            Er::dispatchException(std::current_exception(), xcptLogger);
        }
    }

    static bool grpcInit() noexcept
    {
        ::grpc_init();
    }

    void addContext() noexcept
    {
        std::lock_guard l(m_runningContexts.lock);
        ++m_runningContexts.count;
    }

    void removeContext() noexcept
    {
        bool needToNotify = false;

        {
            std::unique_lock l(m_runningContexts.lock);
            --m_runningContexts.count;

            if (m_runningContexts.count == 0)
                needToNotify = true;
        }

        if (needToNotify)
            m_runningContexts.cv.notify_all();
    }

    void waitRunningContexts()
    {
        while (m_runningContexts.count > 0)
        {
            ClientTrace2(m_log.get(), "{}.SystemInfoClientImpl::waitRunningContexts(): there are {} running contexts yet", Er::Format::ptr(this), m_runningContexts.count);

            std::unique_lock l(m_runningContexts.lock);
            m_runningContexts.cv.wait(l, [this]() { return (m_runningContexts.count <= 0); });
        }

        ClientTrace2(m_log.get(), "{}.SystemInfoClientImpl::waitRunningContexts(): no more running contexts", Er::Format::ptr(this));
    }

    const bool m_grpcReady;
    const std::unique_ptr<erebus::SystemInfo::Stub> m_stub;
    Log::ILogger::Ptr m_log;
    
    struct RunningContexts
    {
        std::mutex lock;
        std::condition_variable cv;
        std::int32_t count = 0;
    };

    RunningContexts m_runningContexts;
};

} // namespace Er::Ipc::Grpc {}