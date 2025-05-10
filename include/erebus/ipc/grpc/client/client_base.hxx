#pragma once

#include <grpcpp/grpcpp.h>

#include <erebus/ipc/grpc/client/iclient.hxx>
#include <erebus/ipc/grpc/protocol.hxx>
#include <erebus/rtl/log.hxx>
#include <erebus/rtl/util/exception_util.hxx>
#include <erebus/rtl/util/unknown_base.hxx>

#include <condition_variable>
#include <mutex>

#include <boost/noncopyable.hpp> 


namespace Er::Ipc::Grpc
{

template <typename _Interface>
class ClientBase
    : public Util::ReferenceCountedBase<Util::ObjectBase<_Interface>>
{
    using Base = Util::ReferenceCountedBase<Util::ObjectBase<_Interface>>;

public:
    ~ClientBase()
    {
        ClientTrace2(m_log.get(), "{}.ClientBase::~ClientBase", Er::Format::ptr(this));

        waitRunningContexts();

        ::grpc_shutdown();
    }

    ClientBase(ChannelPtr channel, Log::LoggerPtr log)
        : m_grpcReady(grpcInit())
        , m_log(log)
    {
        ClientTrace2(m_log.get(), "{}.ClientBase::ClientBase", Er::Format::ptr(this));
    }

protected:
    struct ContextBase
        : public boost::noncopyable
    {
        ~ContextBase() noexcept
        {
            ClientTrace2(m_log, "{}.ContextBase::~ContextBase()", Er::Format::ptr(this));
            m_owner->removeContext();
        }

        ContextBase(ClientBase* owner, Er::Log::ILogger* log) noexcept
            : m_owner(owner)
            , m_log(log)
        {
            ClientTrace2(m_log, "{}.ContextBase::ContextBase()", Er::Format::ptr(this));
            owner->addContext();
        }

        grpc::ClientContext grpcContext;

    protected:
        ClientBase* const m_owner;
        Er::Log::ILogger* const m_log;
    };

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
            ClientTrace2(m_log.get(), "{}.ClientBase::waitRunningContexts(): there are {} running contexts yet", Er::Format::ptr(this), m_runningContexts.count);

            std::unique_lock l(m_runningContexts.lock);
            m_runningContexts.cv.wait(l, [this]() { return (m_runningContexts.count <= 0); });
        }

        ClientTrace2(m_log.get(), "{}.ClientBase::waitRunningContexts(): no more running contexts", Er::Format::ptr(this));
    }

    const bool m_grpcReady;
    Log::LoggerPtr m_log;
    
    struct RunningContexts
    {
        std::mutex lock;
        std::condition_variable cv;
        std::int32_t count = 0;
    };

    RunningContexts m_runningContexts;
};


} // namespace Er::Ipc::Grpc {}
