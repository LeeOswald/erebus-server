#pragma once


#include <erebus/erebus.grpc.pb.h>

#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>
#include <erebus/util/random.hxx>
#include <erebus-srv/erebus-srv.hxx>

#include "auth.hxx"
#include "rpc.hxx"

#include <condition_variable>
#include <mutex>
#include <thread>

namespace Er
{

namespace Server
{

namespace Private
{


class EREBUSSRV_EXPORT ServiceBase
    : public Er::NonCopyable
{
public:
    ~ServiceBase();
    explicit ServiceBase(const Params* params);

    virtual void start();

protected:
    virtual grpc::Service* service() = 0;
    virtual void createRpcs() = 0;
    
    virtual void handleRpcs();
    virtual void processRpcs();

    static void genericDone(Er::Server::Private::Rpc::RpcBase& rpc, bool rpcCancelled);

    std::string getContextUserMapping(grpc::ServerContext* context) const;
    std::string makeTicket() const;

    static void marshalException(erebus::GenericReply* reply, const std::exception& e);
    static void marshalException(erebus::GenericReply* reply, const Er::Exception& e);
    static void marshalException(erebus::GenericReply* reply, Result code, std::string_view message);

    static Er::PropertyBag unmarshalArgs(const erebus::ServiceRequest* request);
    static void marshalReplyProps(const Er::PropertyBag& props, erebus::ServiceReply* reply);

    const size_t kTicketLength = 64;
    const std::string_view kTicketChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";

    bool m_stop = false;
    Params m_params;
    bool m_local;
    std::shared_ptr<AuthMetadataProcessor> m_authProcessor;
    std::unique_ptr<grpc::ServerCompletionQueue> m_queue;
    std::unique_ptr<grpc::Server> m_server;
    std::mutex m_mutex;
    std::condition_variable m_incoming;
    Er::Server::Private::Rpc::TagList m_incomingTags;
    std::unique_ptr<std::thread> m_receiverWorker;
    std::unique_ptr<std::thread> m_processorWorker;
};


} // namespace Private {}

} // namespace Server {}

} // namespace Er {}