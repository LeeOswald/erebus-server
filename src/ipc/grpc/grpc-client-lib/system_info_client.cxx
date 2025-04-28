#include <grpcpp/grpcpp.h>

#include <protobuf/system_info.grpc.pb.h>
#include <erebus/ipc/grpc/client/isystem_info_client.hxx>
#include <erebus/ipc/grpc/protocol.hxx>
#include <erebus/rtl/util/exception_util.hxx>

#include "trace.hxx"

#include <condition_variable>
#include <mutex>

#include <boost/noncopyable.hpp> 


namespace Er::Ipc::Grpc
{

namespace
{

class SystemInfoClientImpl
    : public ISystemInfoClient
    , public std::enable_shared_from_this<SystemInfoClientImpl>
{
    struct PrivateTag {};

public:
    ~SystemInfoClientImpl()
    {
        ClientTrace2(m_log.get(), "{}.SystemInfoClientImpl::~SystemInfoClientImpl", Er::Format::ptr(this));

        waitRunningContexts();

        ::grpc_shutdown();
    }

    SystemInfoClientImpl(PrivateTag, ChannelPtr channel, Log::ILogger::Ptr log)
        : m_grpcReady(grpcInit())
        , m_stub(erebus::SystemInfo::NewStub(channel))
        , m_log(log)
    {
        ClientTrace2(m_log.get(), "{}.SystemInfoClientImpl::SystemInfoClientImpl", Er::Format::ptr(this));
    }

    static ISystemInfoClient::Ptr make(ChannelPtr channel, Log::ILogger::Ptr log)
    {
        return std::make_shared<SystemInfoClientImpl>(PrivateTag{}, channel, log);
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
        ClientTraceIndent2(m_log.get(), "{}.SystemInfoClientImpl::ping(size={})", Er::Format::ptr(this), ping.payload.size());

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

    void getSystemInfo(const std::string& pattern, ISystemInfoCompletion::Ptr handler) override
    {
        ClientTraceIndent2(m_log.get(), "{}.SystemInfoClientImpl::getSystemInfo(pattern={})", Er::Format::ptr(this), pattern);

        new PropertyStreamReader(this, m_log.get(), m_stub.get(), pattern, handler);
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
        ~PingContext()
        {
            ClientTrace2(m_log, "{}.PingContext::~PingContext()", Er::Format::ptr(this));
        }

        PingContext(SystemInfoClientImpl* owner, Er::Log::ILogger* log, PingMessage&& ping, IPingCompletion::Ptr handler)
            : ContextBase(owner, log)
            , handler(handler)
            , ping(std::move(ping))
        {
            ClientTrace2(m_log, "{}.PingContext::PingContext()", Er::Format::ptr(this));

            request.set_timestamp(this->ping.timestamp.value());
            request.set_sequence(this->ping.sequence);
            request.set_payload(this->ping.payload.bytes());
        }

        IPingCompletion::Ptr handler;
        PingMessage ping;
        erebus::PingMessage request;
        erebus::PingMessage reply;
    };

    struct PropertyStreamReader final
        : public grpc::ClientReadReactor<erebus::Property>
        , public ContextBase
    {
        ~PropertyStreamReader()
        {
            ClientTrace2(m_log, "{}.PropertyStreamReader::~PropertyStreamReader()", Er::Format::ptr(this));
        }

        PropertyStreamReader(
            SystemInfoClientImpl* owner, 
            Er::Log::ILogger* log,
            erebus::SystemInfo::Stub* stub,
            const std::string& pattern, 
            ISystemInfoCompletion::Ptr handler
        )
            : ContextBase(owner, log)
            , m_handler(handler)
        {
            ClientTrace2(m_log, "{}.PropertyStreamReader::PropertyStreamReader()", Er::Format::ptr(this));

            m_request.set_propertynamepattern(pattern);

            stub->async()->GetSystemInfo(&grpcContext, &m_request, this);
            StartRead(&m_reply);
            StartCall();
        }

    private:
        void OnReadDone(bool ok) override
        {
            ClientTraceIndent2(m_log, "{}.PropertyStreamReader::OnReadDone({})", Er::Format::ptr(this), ok);

            if (!ok)
                return;

            Er::Util::ExceptionLogger xcptLogger(m_log);

            try
            {
                auto prop = unmarshalProperty(m_reply);
                if (m_handler->onProperty(std::move(prop)) == CallbackResult::Cancel)
                {
                    ErLogWarning2(m_log, "Canceling the request");
                    grpcContext.TryCancel();
                }
            }
            catch (...)
            {
                Er::dispatchException(std::current_exception(), xcptLogger);
            }

            // we have to drain the completion queue even if we cancel
            StartRead(&m_reply);
        }

        void OnDone(const grpc::Status& status) override
        {
            {
                ClientTraceIndent2(m_log, "{}.PropertyStreamReader::OnDone({})", Er::Format::ptr(this), int(status.error_code()));

                Er::Util::ExceptionLogger xcptLogger(m_log);

                try
                {
                    if (!status.ok())
                    {
                        auto resultCode = mapGrpcStatus(status.error_code());
                        auto errorMsg = status.error_message();
                        ErLogError2(m_log, "Stream from terminated with an error: {} ({})", resultCode, errorMsg);

                        m_handler->onError(resultCode, std::move(errorMsg));
                    }
                }
                catch (...)
                {
                    Er::dispatchException(std::current_exception(), xcptLogger);
                }
            }

            m_handler.reset();

            delete this;
        }

        ISystemInfoCompletion::Ptr m_handler;
        erebus::SystemInfoRequest m_request;
        erebus::Property m_reply;
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
        return true;
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

} // namespace {}


ISystemInfoClient::Ptr createSystemInfoClient(ChannelPtr channel, Log::ILogger::Ptr log)
{
    return SystemInfoClientImpl::make(channel, log);
}


} // namespace Er::Ipc::Grpc {}