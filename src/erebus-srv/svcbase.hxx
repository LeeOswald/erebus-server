#pragma once


#include <erebus/erebus.grpc.pb.h>

#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>
#include <erebus-srv/erebus-srv.hxx>

#include "rpc.hxx"

#include <condition_variable>
#include <mutex>
#include <thread>

namespace Erp
{

namespace Server
{


class EREBUSSRV_EXPORT ServiceBase
    : public Er::NonCopyable
{
public:
    ~ServiceBase();
    explicit ServiceBase(const Er::Server::Params* params);

    virtual void start();

protected:
    virtual grpc::Service* service() = 0;
    virtual void createRpcs() = 0;
    
    virtual void receiveRpcs();
    virtual void processRpcs();

    static void genericDone(Erp::Server::Rpc::RpcBase& rpc, bool rpcCancelled);

    static void marshalException(erebus::GenericReply* reply, const std::exception& e);
    static void marshalException(erebus::GenericReply* reply, const Er::Exception& e);
    static void marshalException(erebus::GenericReply* reply, Er::Result code, std::string_view message);

    static Er::PropertyBag unmarshalArgs(const erebus::ServiceRequest* request);
    static void marshalReplyProps(const Er::PropertyBag& props, erebus::ServiceReply* reply);

    bool m_stop = false;
    Er::Server::Params m_params;
    bool m_local;
    std::unique_ptr<grpc::ServerCompletionQueue> m_queue;
    std::unique_ptr<grpc::Server> m_server;
    std::mutex m_mutex;
    std::condition_variable m_incoming;
    Erp::Server::Rpc::TagList m_incomingTags;
    std::unique_ptr<std::thread> m_receiverWorker;
    std::unique_ptr<std::thread> m_processorWorker;
};



} // namespace Server {}

} // namespace Erp {}