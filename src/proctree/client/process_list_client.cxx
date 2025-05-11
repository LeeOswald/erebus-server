#include <protobuf/proctree.grpc.pb.h>

#include "../trace.hxx"

#include <erebus/ipc/grpc/client/client_base.hxx>
#include <erebus/proctree/client/iprocess_list_client.hxx>
#include <erebus/proctree/protocol.hxx>

namespace Er::ProcessTree
{

namespace
{

class ProcessListClientImpl final
    : public Ipc::Grpc::ClientBase<IProcessListClient>
{
    using Base = ClientBase<IProcessListClient>;

public:
    ~ProcessListClientImpl()
    {
        ProctreeTrace2(m_log.get(), "{}.ProcessListClientImpl::~ProcessListClientImpl", Er::Format::ptr(this));
    }

    ProcessListClientImpl(Ipc::Grpc::ChannelPtr channel, Log::LoggerPtr log)
        : Base(channel, log)
        , m_stub(erebus::ProcessList::NewStub(channel))
    {
        ProctreeTrace2(m_log.get(), "{}.ProcessListClientImpl::ProcessListClientImpl", Er::Format::ptr(this));
    }

    void getProcessProperties(Pid pid, const ProcessProperties::Mask& required, GetProcessPropsCompletionPtr completion) override
    {
        ProctreeTrace2(m_log.get(), "{}.ProcessListClientImpl::getProcessProperties(pid={})", Er::Format::ptr(this), pid);

        auto ctx = std::make_shared<GetProcessPropertiesContext>(this, m_log.get(), pid, required, completion);
        
        m_stub->async()->GetProcessProps(
            &ctx->grpcContext,
            &ctx->request,
            &ctx->reply,
            [this, ctx](grpc::Status status)
            {
                completeGetProcessProperties(ctx, status);
            });
    }

private:
    struct GetProcessPropertiesContext
        : public ContextBase
    {
        ~GetProcessPropertiesContext()
        {
            ProctreeTrace2(m_log, "{}.GetProcessPropertiesContext::~GetProcessPropertiesContext()", Er::Format::ptr(this));
        }

        GetProcessPropertiesContext(
            ProcessListClientImpl* owner, 
            Er::Log::ILogger* log, 
            Pid pid, 
            const ProcessProperties::Mask& required, 
            Er::ReferenceCountedPtr<IGetProcessPropsCompletion> handler
        )
            : ContextBase(owner, log)
            , handler(handler)
        {
            ProctreeTrace2(m_log, "{}.GetProcessPropertiesContext::GetProcessPropertiesContext()", Er::Format::ptr(this));

            request.set_pid(pid);
            marshalProcessPropertyMsk(request, required);
        }

        Er::ReferenceCountedPtr<IGetProcessPropsCompletion> handler;
        erebus::ProcessPropsRequest request;
        erebus::ProcessPropsReply reply;
    };

    void completeGetProcessProperties(std::shared_ptr<GetProcessPropertiesContext> ctx, grpc::Status status)
    {
        ProctreeTraceIndent2(m_log.get(), "{}.ProcessListClientImpl::completeGetProcessProperties", Er::Format::ptr(this));

        Er::Util::ExceptionLogger xcptLogger(m_log.get());

        try
        {
            if (!status.ok())
            {
                // transport failure or something
                ErLogError2(m_log.get(), "GetProcessProperties() failed for {}: {} ({})", ctx->grpcContext.peer(), int(status.error_code()), status.error_message());

                return ctx->handler->onError(status);
            }
            else
            {
                Timings timings;

                if (ctx->reply.has_header())
                {
                    auto& hdr = ctx->reply.header();
                    if (hdr.has_exception())
                    {
                        auto e = Ipc::Grpc::unmarshalException(hdr.exception());
                        ProctreeTrace2(m_log.get(), "GetProcessProperties() returned an error: {}", e.message());
                        return ctx->handler->onException(std::move(e));
                    }

                    if (hdr.has_timestamp())
                    {
                        auto now = Time::now();
                        timings.rtt = now - hdr.timestamp();
                    }

                    if (hdr.has_duration())
                    {
                        timings.processing = hdr.duration();
                    }
                }

                if (ctx->reply.has_props())
                {
                    auto props = unmarshalProcessProperties(ctx->reply.props());
                    return ctx->handler->onReply(std::move(props), timings);
                }
                
                // looks like an empty reply
                ctx->handler->onReply(ProcessProperties{}, timings);
            }
        }
        catch (...)
        {
            Er::dispatchException(std::current_exception(), xcptLogger);
        }
    }

    const std::unique_ptr<erebus::ProcessList::Stub> m_stub;
};

} // namespace {}


ER_PROCTREE_EXPORT ProcessListClientPtr createProcessListClient(Ipc::Grpc::ChannelPtr channel, Log::LoggerPtr log)
{
    return ProcessListClientPtr{ new ProcessListClientImpl(channel, log) };
}


} // namespace Er::ProcessTree {}
