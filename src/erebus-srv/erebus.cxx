#include "erebus-version.h"
#include "svcbase.hxx"

#include <erebus/util/exceptionutil.hxx>

#include <shared_mutex>
#include <unordered_map>

#include <erebus/trace.hxx>

namespace Er::Server
{

namespace
{


class ErebusService final
    : public Er::Server::IServer
    , public Er::Server::IServiceContainer
    , public Erp::Server::ServiceBase
{
public:
    ~ErebusService() 
    {
        TraceMethod("ErebusService");
    }

    explicit ErebusService(const Params& params)
        : Erp::Server::ServiceBase(&m_service, params)
    {
        TraceMethod("ErebusService");
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
            ErThrow(Er::format("Service for [{}] is already registered", id));
        
        Er::Log::info(m_params.log, "Registered service {} for [{}]", Format::ptr(service), id);

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
                Er::Log::info(m_params.log, "Unregistered service {}", Format::ptr(service));

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
            Er::Log::error(m_params.log, "Service {} is not registered", Format::ptr(service));
    }

private:
    void createRpcs(grpc::ServerCompletionQueue* cq) override
    {
        TraceMethod("ErebusService");

        createGenericRpc(cq);
        createGenericStream(cq);
    }

    
    void createGenericRpc(grpc::ServerCompletionQueue* cq)
    {
        TraceMethod("ErebusService");
        Erp::Server::Rpc::UnaryRpcHandlers<erebus::Erebus::AsyncService, erebus::ServiceRequest, erebus::ServiceReply> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createGenericRpc, this, cq);

        rpcHandlers.processIncomingRequest = [this](Erp::Server::Rpc::RpcBase& rpc, const google::protobuf::Message* message) 
        { 
            ErebusService::processGenericRpc(rpc, message); 
        };

        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestGenericRpc;

        new Erp::Server::Rpc::UnaryRpc<erebus::Erebus::AsyncService, erebus::ServiceRequest, erebus::ServiceReply>(&m_service, cq, rpcHandlers);
    }

    void createGenericStream(grpc::ServerCompletionQueue* cq)
    {
        TraceMethod("ErebusService");
        Erp::Server::Rpc::ServerStreamingRpcHandlers<erebus::Erebus::AsyncService, erebus::ServiceRequest, erebus::ServiceReply> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createGenericStream, this, cq);

        rpcHandlers.processIncomingRequest = [this](Erp::Server::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
        {
            ErebusService::processGenericStream(rpc, message);
        };

        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestGenericStream;

        new Erp::Server::Rpc::ServerStreamingRpc<erebus::Erebus::AsyncService, erebus::ServiceRequest, erebus::ServiceReply>(&m_service, cq, rpcHandlers);
    }

    void processGenericRpc(Erp::Server::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
    {
        TraceMethod("ErebusService");
        auto request = static_cast<const erebus::ServiceRequest*>(message);
        erebus::ServiceReply response;

        auto& id = request->request();
        
        IService* service = nullptr;
        {
            std::shared_lock l(m_servicesLock);

            auto it = m_services.find(id);
            if (it != m_services.end())
            {
                service = it->second;
            }
        }

        if (!service)
        {
            Er::Log::error(m_params.log, "No handlers for [{}]", id);
            rpc.finishWithError(grpc::Status(grpc::UNIMPLEMENTED, "Not implemented"));
            return;
        }
        
        try
        {
            auto args = unmarshalArgs(request);
            auto result = service->request(id, args);
            marshalReplyProps(result, &response);
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

    void processGenericStream(Erp::Server::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
    {
        TraceMethod("ErebusService");
        auto request = static_cast<const erebus::ServiceRequest*>(message);
        
        auto& id = request->request();

        IService* service = nullptr;
        {
            std::shared_lock l(m_servicesLock);

            auto it = m_services.find(id);
            if (it != m_services.end())
            {
                service = it->second;
            }
        }

        if (!service)
        {
            Er::Log::error(m_params.log, "No handlers for [{}]", id);
            rpc.finishWithError(grpc::Status(grpc::UNIMPLEMENTED, "Not implemented"));
            return;
        }

        try
        {
            auto args = unmarshalArgs(request);
            auto streamId = service->beginStream(id, args);
            bool stop = false;
            do
            {
                erebus::ServiceReply response;
                try
                {
                    auto item = service->next(streamId);
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

                    marshalException(&response, e);
                    stop = true;
                }
                catch (std::exception& e)
                {
                    Er::Util::logException(m_params.log, Er::Log::Level::Error, e);

                    marshalException(&response, e);
                    stop = true;
                }

                rpc.sendResponse(&response);

            } while (!stop);
            
            service->endStream(streamId);
        }
        catch (Er::Exception& e)
        {
            Er::Util::logException(m_params.log, Er::Log::Level::Error, e);

            erebus::ServiceReply response;
            marshalException(&response, e);
            rpc.sendResponse(&response);
        }
        catch (std::exception& e)
        {
            Er::Util::logException(m_params.log, Er::Log::Level::Error, e);

            erebus::ServiceReply response;
            marshalException(&response, e);
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


IServer::Ptr EREBUSSRV_EXPORT create(const Params& params)
{
    auto result = std::make_unique<ErebusService>(params);
    result->start();
    return result;
}


} // namespace Er::Server {}