#include <protobuf/system_info.grpc.pb.h>

#include <erebus/ipc/grpc/protocol.hxx>
#include <erebus/ipc/grpc/server/iservice.hxx>
#include <erebus/rtl/util/exception_util.hxx>
#include <erebus/rtl/util/unknown_base.hxx>
#include <erebus/server/system_info.hxx>

#include "trace.hxx"


namespace Er::Ipc::Grpc
{

namespace
{

class SystemInfoImpl
    : public Util::DisposableBase<Util::ObjectBase<IService, IDisposable>>
    , public erebus::SystemInfo::CallbackService
{
    using Base = Util::DisposableBase<Util::ObjectBase<IService, IDisposable>>;

public:
    ~SystemInfoImpl()
    {
        ServerTrace2(m_log.get(), "{}.SystemInfoImpl::~SystemInfoImpl", Er::Format::ptr(this));
    }

    SystemInfoImpl(Log::ILogger::Ptr log, IDisposableParent* owner)
        : Base(owner)
        , m_log(log)
    {
        ServerTrace2(m_log.get(), "{}.SystemInfoImpl::SystemInfoImpl", Er::Format::ptr(this));
    }

    ::grpc::Service* grpc() noexcept override
    {
        return this;
    }

    std::string_view name() const noexcept override
    {
        return "SystemInfo";
    }

    grpc::ServerWriteReactor<erebus::Property>* GetSystemInfo(grpc::CallbackServerContext* context, const erebus::SystemInfoRequest* request) override
    {
        ServerTraceIndent2(m_log.get(), "{}.SystemInfoImpl::GetSystemInfo", Er::Format::ptr(this));

        auto reactor = std::make_unique<SystemInfoReplyReactor>(m_log);
        if (context->IsCancelled()) [[unlikely]]
        {
            ErLogWarning2(m_log.get(), "GetSystemInfo canceled");
            reactor->Finish(grpc::Status::CANCELLED);
            return reactor.release();
        }

        auto& pattern = request->propertynamepattern();
        ErLogInfo2(m_log.get(), "GetSystemInfo(pattern={}) from {}", pattern, context->peer());

        Er::Util::ExceptionLogger xcptHandler(m_log.get());
        try
        {
            auto props = SystemInfo::get(pattern);
            reactor->Begin(std::move(props));
        }
        catch (...)
        {
            Er::dispatchException(std::current_exception(), xcptHandler);

            reactor->Finish(grpc::Status(grpc::INTERNAL, xcptHandler.lastError()));
        }

        return reactor.release();
    }

    grpc::ServerUnaryReactor* Ping(grpc::CallbackServerContext* context, const erebus::PingMessage* request, erebus::PingMessage* reply) override
    {
        ServerTraceIndent2(m_log.get(), "{}.SystemInfoImpl::Ping(peer={})", Er::Format::ptr(this), context->peer());

        auto reactor = std::make_unique<PingReplyReactor>(m_log);
        if (context->IsCancelled()) [[unlikely]]
        {
            ErLogWarning2(m_log.get(), "Ping canceled");
            reactor->Finish(grpc::Status::CANCELLED);
            return reactor.release();
        }

        auto timestamp = request->timestamp();
        auto sequence = request->sequence();
        auto& payload = request->payload();

        ErLogInfo2(m_log.get(), "Ping #{} from {} with {} bytes of data", sequence, context->peer(), payload.size());

        reply->set_timestamp(timestamp);
        reply->set_sequence(sequence);
        reply->set_payload(payload);

        reactor->Finish(grpc::Status::OK);
        return reactor.release();
    }

private:
    class PingReplyReactor
        : public grpc::ServerUnaryReactor
    {
    public:
        ~PingReplyReactor()
        {
            ServerTrace2(m_log.get(), "{}.PingReplyReactor::~PingReplyReactor", Er::Format::ptr(this));
        }

        PingReplyReactor(Log::ILogger::Ptr log) noexcept
            : m_log(log)
        {
            ServerTrace2(m_log.get(), "{}.PingReplyReactor::PingReplyReactor", Er::Format::ptr(this));
        }

    private:
        void OnDone() override
        {
            ServerTraceIndent2(m_log.get(), "{}.PingReplyReactor::OnDone", Er::Format::ptr(this));

            delete this;
        }

        void OnCancel() override
        {
            ServerTrace2(m_log.get(), "{}.PingReplyReactor::OnCancel", Er::Format::ptr(this));
        }

        Log::ILogger::Ptr m_log;
    };

    class SystemInfoReplyReactor
        : public grpc::ServerWriteReactor<erebus::Property>
    {
    public:
        ~SystemInfoReplyReactor()
        {
            ServerTrace2(m_log.get(), "{}.SystemInfoReplyReactor::~SystemInfoReplyReactor", Er::Format::ptr(this));
        }

        SystemInfoReplyReactor(Log::ILogger::Ptr log)
            : m_log(log)
            , m_bag()
            , m_next(m_bag.end())
        {
            ServerTrace2(m_log.get(), "{}.SystemInfoReplyReactor::SystemInfoReplyReactor", Er::Format::ptr(this));
        }

        void Begin(PropertyBag&& reply)
        {
            ServerTraceIndent2(m_log.get(), "{}.SystemInfoReplyReactor::Begin(count={})", Er::Format::ptr(this), reply.size());

            m_bag = std::move(reply);
            m_next = m_bag.begin();

            Continue();
        }

    private:
        void OnWriteDone(bool ok) override
        {
            ServerTraceIndent2(m_log.get(), "{}.SystemInfoReplyReactor::OnWriteDone", Er::Format::ptr(this));

            if (!ok)
                Finish(grpc::Status(grpc::StatusCode::CANCELLED, "Operation canceled"));
            else
                Continue();
        }

        void OnDone() override
        {
            ServerTraceIndent2(m_log.get(), "{}.SystemInfoReplyReactor::OnDone", Er::Format::ptr(this));

            delete this;
        }

        void OnCancel() override
        {
            ServerTrace2(m_log.get(), "{}.SystemInfoReplyReactor::OnCancel", Er::Format::ptr(this));
        }

        void Continue()
        {
            ServerTraceIndent2(m_log.get(), "{}.SystemInfoReplyReactor::Continue", Er::Format::ptr(this));

            if (m_next != m_bag.end())
            {
                marshalProperty(*m_next, m_reply);
                ++m_next;

                StartWrite(&m_reply);
            }
            else
            {
                ServerTrace2(m_log.get(), "End of stream", Er::Format::ptr(this));
                Finish(grpc::Status::OK);
            }
        }

        Log::ILogger::Ptr m_log;
        PropertyBag m_bag;
        PropertyBag::const_iterator m_next;
        erebus::Property m_reply;
    };

    Log::ILogger::Ptr m_log;
};


} // namespace {}


IService* createSystemInfoService(Log::ILogger::Ptr log, IDisposableParent* owner)
{
    return new SystemInfoImpl(log, owner);
}

} // namespace Er::Ipc::Grpc {}