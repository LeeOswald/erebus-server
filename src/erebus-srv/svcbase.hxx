#pragma once


#include <erebus/erebus.grpc.pb.h>

#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>
#include <erebus-srv/erebus-srv.hxx>

#include "rpc.hxx"

#include <condition_variable>
#include <mutex>
#include <thread>

namespace Erp::Server
{


class EREBUSSRV_EXPORT ServiceBase
    : public Er::NonCopyable
{
public:
    ~ServiceBase();
    explicit ServiceBase(grpc::Service* service, const Er::Server::Params& params);

    virtual void start();

protected:
    struct TagQueue
        : public Er::NonCopyable
    {
        std::mutex mutex;
        std::condition_variable available;
        Erp::Server::Rpc::TagList tags;
    };

    struct RpcReceiver
        : public Er::NonCopyable
    {
        ServiceBase* owner;
        Er::Log::ILog* log;
        std::unique_ptr<grpc::ServerCompletionQueue> cq;
        TagQueue& incoming;
        std::jthread thread;

        ~RpcReceiver()
        {
            thread.request_stop();
            cq->Shutdown();
            // drain the CQ
            void* tag = nullptr;
            bool ok = false;
            while (cq->Next(&tag, &ok))
            {
            }
        }

        RpcReceiver(ServiceBase* owner, Er::Log::ILog* log, std::unique_ptr<grpc::ServerCompletionQueue>&& cq, TagQueue& incoming)
            : owner(owner)
            , log(log)
            , cq(std::move(cq))
            , incoming(incoming)
            , thread([this](std::stop_token stop) { run(stop); })
        {
        }

        void run(std::stop_token stop);
    };

    struct RpcHandler
        : public Er::NonCopyable
    {
        ServiceBase* owner;
        Er::Log::ILog* log;
        TagQueue& incoming;
        std::jthread thread1;
        std::jthread thread2;

        ~RpcHandler() = default;

        RpcHandler(ServiceBase* owner, Er::Log::ILog* log, TagQueue& incoming)
            : owner(owner)
            , log(log)
            , incoming(incoming)
            , thread1([this](std::stop_token stop) { run(stop); })
            , thread2([this](std::stop_token stop) { run(stop); })
        {
        }

        void run(std::stop_token stop);
    };
    
    virtual void createRpcs(grpc::ServerCompletionQueue* cq) = 0;
    
    static void genericDone(Erp::Server::Rpc::RpcBase& rpc, bool rpcCancelled);

    static void marshalException(erebus::ServiceReply* reply, const std::exception& e);
    static void marshalException(erebus::ServiceReply* reply, const Er::Exception& e);
    static void marshalException(erebus::ServiceReply* reply, Er::Result code, std::string_view message);

    static Er::PropertyBag unmarshalArgs(const erebus::ServiceRequest* request);
    static void marshalReplyProps(const Er::PropertyBag& props, erebus::ServiceReply* reply);

    grpc::Service* m_service;
    Er::Server::Params m_params;
    TagQueue m_incoming;
    std::unique_ptr<grpc::Server> m_server;
    std::unique_ptr<RpcReceiver> m_receiver;
    std::vector<std::unique_ptr<RpcHandler>> m_handlers;
};



} // namespace Erp::Server {}