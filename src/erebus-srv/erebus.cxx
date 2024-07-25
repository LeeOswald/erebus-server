#include "erebus-version.h"
#include "svcbase.hxx"

#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/format.hxx>

#include <shared_mutex>
#include <unordered_map>

namespace Er
{

namespace Server
{

namespace Private
{

namespace
{


class ErebusService final
    : public Er::Server::Private::IServer
    , public Er::Server::IServiceContainer
    , public ServiceBase
{
public:
    ~ErebusService() 
    {
    }

    explicit ErebusService(const Params* params)
        : ServiceBase(params)
    {
    }

    IServiceContainer* serviceContainer() override
    {
        return this;
    }

    void registerService(std::string_view request, IService* service) override
    {
        std::lock_guard l(m_servicesLock);

        std::string id(request);
        auto it = m_services.find(id);
        if (it != m_services.end())
            throwGenericError(Er::Util::format("Service for [%s] is already registered", id.c_str()));
        
        ErLogInfo(m_params.log, "Registered service %p for [%s]", service, id.c_str());

        m_services.insert({ std::move(id), service });
    }

    void unregisterService(IService* service) override
    {
        std::lock_guard l(m_servicesLock);
        
        bool success = false;
        for (auto it = m_services.begin(); it != m_services.end();)
        {
            if (it->second == service)
            {
                ErLogInfo(m_params.log, "Unregistered service %p", service);

                auto next = std::next(it);
                m_services.erase(it);
                it = next;

                success = true;
            }
            else
            {
                ++it;
            }
        }

        if (!success)
            ErLogError(m_params.log, "Service % p is not registered", service);
    }

private:
    grpc::Service* service() override
    {
        return &m_service;
    }

    void createRpcs() override
    {
        createAllocateSessionRpc();
        createDeleteSessionRpc();
        createGenericRpc();
        createGenericStream();
    }

    void createAllocateSessionRpc()
    {
        Er::Server::Private::Rpc::UnaryRpcHandlers<erebus::Erebus::AsyncService, erebus::AllocateSessionRequest, erebus::AllocateSessionReply> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createAllocateSessionRpc, this);

        rpcHandlers.processIncomingRequest = [this](Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message) { ErebusService::processAllocateSessionRpc(rpc, message); };
        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestAllocateSession;

        new Er::Server::Private::Rpc::UnaryRpc<erebus::Erebus::AsyncService, erebus::AllocateSessionRequest, erebus::AllocateSessionReply>(&m_service, m_queue.get(), rpcHandlers);
    }

    void processAllocateSessionRpc(Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
    {
        auto request = static_cast<const erebus::AllocateSessionRequest*>(message);
        erebus::AllocateSessionReply response;

        std::shared_lock l(m_servicesLock);
        
        auto& serviceId = request->request();

        auto it = m_services.find(serviceId);
        if (it == m_services.end())
        {
            m_params.log->writef(Er::Log::Level::Error, "No handlers for [%s]", serviceId.c_str());
            rpc.finishWithError(grpc::Status(grpc::UNIMPLEMENTED, "Not implemented"));
            return;
        }

        auto service = it->second;

        try
        {
            auto sessionId = service->allocateSession();
            response.set_sessionid(sessionId);
        }
        catch (Er::Exception& e)
        {
            Er::Util::logException(m_params.log, Er::Log::Level::Error, e);

            marshalException(response.mutable_header(), e);
        }
        catch (std::exception& e)
        {
            Er::Util::logException(m_params.log, Er::Log::Level::Error, e);

            marshalException(response.mutable_header(), e);
        }
        
        rpc.sendResponse(&response);
    }

    void createDeleteSessionRpc()
    {
        Er::Server::Private::Rpc::UnaryRpcHandlers<erebus::Erebus::AsyncService, erebus::DeleteSessionRequest, erebus::GenericReply> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createDeleteSessionRpc, this);

        rpcHandlers.processIncomingRequest = [this](Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message) { ErebusService::processDeleteSessionRpc(rpc, message); };
        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestDeleteSession;

        new Er::Server::Private::Rpc::UnaryRpc<erebus::Erebus::AsyncService, erebus::DeleteSessionRequest, erebus::GenericReply>(&m_service, m_queue.get(), rpcHandlers);
    }

    void processDeleteSessionRpc(Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
    {
        auto request = static_cast<const erebus::DeleteSessionRequest*>(message);
        erebus::GenericReply response;

        std::shared_lock l(m_servicesLock);
        
        auto& serviceId = request->request();

        auto it = m_services.find(serviceId);
        if (it == m_services.end())
        {
            m_params.log->writef(Er::Log::Level::Error, "No handlers for [%s]", serviceId.c_str());
            rpc.finishWithError(grpc::Status(grpc::UNIMPLEMENTED, "Not implemented"));
            return;
        }

        auto service = it->second;

        try
        {
            auto sessionId = request->sessionid();
            service->deleteSession(sessionId);
            
        }
        catch (Er::Exception& e)
        {
            Er::Util::logException(m_params.log, Er::Log::Level::Error, e);

            marshalException(&response, e);
        }
        catch (std::exception& e)
        {
            Er::Util::logException(m_params.log, Er::Log::Level::Error, e);

            marshalException(&response, e);
        }
        
        rpc.sendResponse(&response);
    }

    void createGenericRpc()
    {
        Er::Server::Private::Rpc::UnaryRpcHandlers<erebus::Erebus::AsyncService, erebus::ServiceRequest, erebus::ServiceReply> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createGenericRpc, this);

        rpcHandlers.processIncomingRequest = [this](Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message) { ErebusService::processGenericRpc(rpc, message); };
        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestGenericRpc;

        new Er::Server::Private::Rpc::UnaryRpc<erebus::Erebus::AsyncService, erebus::ServiceRequest, erebus::ServiceReply>(&m_service, m_queue.get(), rpcHandlers);
    }

    void processGenericRpc(Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
    {
        auto request = static_cast<const erebus::ServiceRequest*>(message);
        erebus::ServiceReply response;

        auto& id = request->request();
        uint32_t sessionId = 0;
        if (request->has_sessionid())
            sessionId = request->sessionid();

        response.set_sessionid(sessionId);

        std::shared_lock l(m_servicesLock);
        
        auto it = m_services.find(id);
        if (it == m_services.end())
        {
            m_params.log->writef(Er::Log::Level::Error, "No handlers for [%s]", id.c_str());
            rpc.finishWithError(grpc::Status(grpc::UNIMPLEMENTED, "Not implemented"));
            return;
        }

        auto service = it->second;

        try
        {
            auto args = unmarshalArgs(request);
            auto result = service->request(id, args, sessionId);
            marshalReplyProps(result, &response);
        }
        catch (Er::Exception& e)
        {
            Er::Util::logException(m_params.log, Er::Log::Level::Error, e);

            marshalException(response.mutable_header(), e);
        }
        catch (std::exception& e)
        {
            Er::Util::logException(m_params.log, Er::Log::Level::Error, e);

            marshalException(response.mutable_header(), e);
        }
        
        rpc.sendResponse(&response);
    }

    void createGenericStream()
    {
        Er::Server::Private::Rpc::ServerStreamingRpcHandlers<erebus::Erebus::AsyncService, erebus::ServiceRequest, erebus::ServiceReply> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createGenericStream, this);

        rpcHandlers.processIncomingRequest = [this](Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message) { ErebusService::processGenericStream(rpc, message); };
        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestGenericStream;

        new Er::Server::Private::Rpc::ServerStreamingRpc<erebus::Erebus::AsyncService, erebus::ServiceRequest, erebus::ServiceReply>(&m_service, m_queue.get(), rpcHandlers);
    }

    void processGenericStream(Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
    {
        auto request = static_cast<const erebus::ServiceRequest*>(message);
        
        auto& id = request->request();
        uint32_t sessionId = 0;
        if (request->has_sessionid())
            sessionId = request->sessionid();

        std::shared_lock l(m_servicesLock);
        
        auto it = m_services.find(id);
        if (it == m_services.end())
        {
            m_params.log->writef(Er::Log::Level::Error, "No handlers for [%s]", id.c_str());
            rpc.finishWithError(grpc::Status(grpc::UNIMPLEMENTED, "Not implemented"));
            return;
        }

        auto service = it->second;

        try
        {
            auto args = unmarshalArgs(request);
            auto streamId = service->beginStream(id, args, sessionId);
            bool stop = false;
            do
            {
                erebus::ServiceReply response;
                if (sessionId)
                        response.set_sessionid(sessionId);

                try
                {
                    auto item = service->next(streamId, sessionId);
                    if (item.empty())
                    {
                        stop = true;
                    }
                    else
                    {
                        marshalReplyProps(item, &response);
                    }
                }
                catch (Er::Exception& e)
                {
                    Er::Util::logException(m_params.log, Er::Log::Level::Error, e);

                    marshalException(response.mutable_header(), e);
                    stop = true;
                }
                catch (std::exception& e)
                {
                    Er::Util::logException(m_params.log, Er::Log::Level::Error, e);

                    marshalException(response.mutable_header(), e);
                    stop = true;
                }

                rpc.sendResponse(&response);

            } while (!stop);
            
            service->endStream(streamId, sessionId);
        }
        catch (Er::Exception& e)
        {
            Er::Util::logException(m_params.log, Er::Log::Level::Error, e);

            erebus::ServiceReply response;
            marshalException(response.mutable_header(), e);
            rpc.sendResponse(&response);
        }
        catch (std::exception& e)
        {
            Er::Util::logException(m_params.log, Er::Log::Level::Error, e);

            erebus::ServiceReply response;
            marshalException(response.mutable_header(), e);
            rpc.sendResponse(&response);
        }
        
        // end of stream
        rpc.sendResponse(nullptr);
    }


    erebus::Erebus::AsyncService m_service;
    std::shared_mutex m_servicesLock;
    std::unordered_map<std::string, IService*> m_services;
};


} // namespace {}


std::shared_ptr<IServer> EREBUSSRV_EXPORT create(const Params* params)
{
    auto result = std::make_shared<ErebusService>(params);
    result->start();
    return result;
}

} // namespace Private {}

} // namespace Server {}

} // namespace Er {}