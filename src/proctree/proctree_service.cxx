#include <protobuf/proctree.grpc.pb.h>

#include <erebus/proctree/protocol.hxx>
#include <erebus/ipc/grpc/server/iservice.hxx>
#include <erebus/rtl/util/exception_util.hxx>
#include <erebus/rtl/util/unknown_base.hxx>

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
        ProctreeTrace2(m_log, "{}.ProctreeService::ProctreeService", Er::Format::ptr(this));
    }

    ::grpc::Service* grpc() noexcept override
    {
        return this;
    }

    std::string_view name() const noexcept override
    {
        return "ProcessList";
    }

private:
    Log::ILogger* m_log;
};


} // namespace {}


} // namespace Er::ProcessTree::Private {}