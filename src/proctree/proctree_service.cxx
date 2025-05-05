#include <protobuf/proctree.grpc.pb.h>

#include <erebus/proctree/protocol.hxx>
#include <erebus/rtl/time.hxx>
#include <erebus/rtl/util/exception_util.hxx>
#include <erebus/rtl/util/unknown_base.hxx>

#include "proctree_service.hxx"
#include "trace.hxx"

namespace Er::ProcessTree::Private
{

namespace
{


class ProctreeService final
    : public Util::ReferenceCountedBase<Util::ObjectBase<Er::Ipc::Grpc::IService>>
    , public erebus::ProcessList::CallbackService
{
    using Base = Util::ReferenceCountedBase<Util::ObjectBase<Er::Ipc::Grpc::IService>>;

public:
    ~ProctreeService()
    {
        ProctreeTrace2(m_log, "{}.ProctreeService::~ProctreeService", Er::Format::ptr(this));
    }

    ProctreeService(Log::ILogger* log)
        : m_log(log)
    {
        ProctreeTraceIndent2(m_log, "{}.ProctreeService::ProctreeService", Er::Format::ptr(this));
    }

    ::grpc::Service* grpc() noexcept override
    {
        return this;
    }

    std::string_view name() const noexcept override
    {
        return "ProcessList";
    }

    grpc::ServerUnaryReactor* GetProcessProps(grpc::CallbackServerContext* context, const erebus::ProcessPropsRequest* request, erebus::ProcessPropsReply* reply) override
    {
        ProctreeTraceIndent2(m_log, "{}.ProctreeService::GetProcessProps", Er::Format::ptr(this));

        auto reactor = std::make_unique<ProcessPropsReplyReactor>(m_log);
        if (context->IsCancelled()) [[unlikely]]
        {
            ErLogWarning2(m_log, "Ping canceled");
            reactor->Finish(grpc::Status::CANCELLED);
            return reactor.release();
        }

        std::optional<Time::ValueType> timestamp;
        std::optional<Time::ValueType> started;
        if (request->has_header())
        {
            timestamp = request->header().timestamp();
            started = Time::now();
        }

        auto pid = request->pid();
        auto mask = unmarshalProcessPropertyMask(*request);

        return reactor.release();
    }

private:
    class ProcessPropsReplyReactor
        : public grpc::ServerUnaryReactor
    {
    public:
        ~ProcessPropsReplyReactor()
        {
            ProctreeTrace2(m_log, "{}.ProcessPropsReplyReactor::~ProcessPropsReplyReactor", Er::Format::ptr(this));
        }

        ProcessPropsReplyReactor(Log::ILogger* log) noexcept
            : m_log(log)
        {
            ProctreeTrace2(m_log, "{}.ProcessPropsReplyReactor::ProcessPropsReplyReactor", Er::Format::ptr(this));
        }

    private:
        void OnDone() override
        {
            ProctreeTraceIndent2(m_log, "{}.ProcessPropsReplyReactor::OnDone", Er::Format::ptr(this));

            delete this;
        }

        void OnCancel() override
        {
            ProctreeTrace2(m_log, "{}.ProcessPropsReplyReactor::OnCancel", Er::Format::ptr(this));
        }

        Log::ILogger* m_log;
    };

    Log::ILogger* m_log;
};


} // namespace {}


Er::Ipc::Grpc::ServicePtr createProcessListService(Er::Log::ILogger* log)
{
    return Er::Ipc::Grpc::ServicePtr{ new ProctreeService(log) };
}

} // namespace Er::ProcessTree::Private {}