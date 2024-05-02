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
        ErLogDebug(m_params.log, ErLogInstance("ErebusService"), "~ErebusService()");
    }

    explicit ErebusService(const Params* params)
        : ServiceBase(params)
    {
        m_authProcessor->addNoAuthMethod("/erebus.Erebus/Init");
        m_authProcessor->addNoAuthMethod("/erebus.Erebus/Authorize");

        ErLogDebug(m_params.log, ErLogInstance("ErebusService"), "ErebusService()");
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
            throw Er::Exception(ER_HERE(), Er::Util::format("Service for [%s] is already registered", id.c_str()));

        
        ErLogInfo(m_params.log, ErLogInstance("ErebusService"), "Registered service %p for [%s]", service, id.c_str());

        m_services.insert({ std::move(id), service });
    }

    void unregisterService(IService* service) override
    {
        std::lock_guard l(m_servicesLock);
        
        for (auto it = m_services.begin(); it != m_services.end();)
        {
            if (it->second == service)
            {
                ErLogInfo(m_params.log, ErLogInstance("ErebusService"), "Unregistered service %p", service);

                auto next = std::next(it);
                m_services.erase(it);
                it = next;
            }
            else
            {
                ++it;
            }
        }

        ErLogError(m_params.log, ErLogInstance("ErebusService"), "Service % p is not registered", service);
    }

private:
    grpc::Service* service() override
    {
        return &m_service;
    }

    void createRpcs() override
    {
        createDisconnectRpc();
        createVersionRpc();
        createInitRpc();
        createAuthorizeRpc();
        createAddUserRpc();
        createRemoveUserRpc();
        createListUsersRpc();
        createExitRpc();
        createAllocateSessionRpc();
        createDeleteSessionRpc();
        createGenericRpc();
        createGenericStream();
    }

    bool checkAuth(Er::Server::Private::Rpc::RpcBase& rpc)
    {
        if (!rpc.getServerContext().auth_context()->IsPeerAuthenticated())
        {
            rpc.finishWithError(grpc::Status(grpc::UNAUTHENTICATED, "Unauthenticated"));
            return false;
        }

        return true;
    }

    void createVersionRpc()
    {
        Er::Server::Private::Rpc::UnaryRpcHandlers<erebus::Erebus::AsyncService, erebus::Void, erebus::ServerVersionReply> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createVersionRpc, this);

        rpcHandlers.processIncomingRequest = [this](Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message) { ErebusService::processVersionRpc(rpc, message); };
        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestVersion;

        new Er::Server::Private::Rpc::UnaryRpc<erebus::Erebus::AsyncService, erebus::Void, erebus::ServerVersionReply>(&m_service, m_queue.get(), rpcHandlers);
    }

    void processVersionRpc(Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
    {
        if (!checkAuth(rpc))
            return;

        auto request = static_cast<const erebus::Void*>(message);

        erebus::ServerVersionReply response;
        response.set_major(ER_VERSION_MAJOR);
        response.set_minor(ER_VERSION_MINOR);
        response.set_patch(ER_VERSION_PATCH);

        rpc.sendResponse(&response);
    }

    void createDisconnectRpc()
    {
        Er::Server::Private::Rpc::UnaryRpcHandlers<erebus::Erebus::AsyncService, erebus::Void, erebus::Void> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createDisconnectRpc, this);

        rpcHandlers.processIncomingRequest = [this](Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message) { ErebusService::processDisconnectRpc(rpc, message); };
        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestDisconnect;

        new Er::Server::Private::Rpc::UnaryRpc<erebus::Erebus::AsyncService, erebus::Void, erebus::Void>(&m_service, m_queue.get(), rpcHandlers);
    }

    void processDisconnectRpc(Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
    {
        if (!checkAuth(rpc))
            return;

        auto tickets = rpc.getServerContext().auth_context()->FindPropertyValues("ticket");

        auto request = static_cast<const erebus::Void*>(message);

        erebus::ServerVersionReply response;
        rpc.sendResponse(&response);

        if (!tickets.empty())
            m_authProcessor->removeTicket(tickets.front().data());
    }

    void createInitRpc()
    {
        Er::Server::Private::Rpc::UnaryRpcHandlers<erebus::Erebus::AsyncService, erebus::InitialRequest, erebus::InitialReply> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createInitRpc, this);

        rpcHandlers.processIncomingRequest = [this](Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message) { ErebusService::processInitRpc(rpc, message); };
        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestInit;

        new Er::Server::Private::Rpc::UnaryRpc<erebus::Erebus::AsyncService, erebus::InitialRequest, erebus::InitialReply>(&m_service, m_queue.get(), rpcHandlers);
    }

    void processInitRpc(Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
    {
        auto request = static_cast<const erebus::InitialRequest*>(message);

        erebus::InitialReply response;
        auto& user = request->user();
        auto u = m_params.userDb->lookup(user);
        if (!u)
        {
            Er::Log::Warning(m_params.log, ErLogInstance("ErebusService")) << "Trying to log in an unknown user " << user;
            marshalException(response.mutable_header(), Result::NotFound, "User not found");
        }
        else
        {
            response.set_salt(u->pwdSalt);
        }

        rpc.sendResponse(&response);
    }

    void createAuthorizeRpc()
    {
        Er::Server::Private::Rpc::UnaryRpcHandlers<erebus::Erebus::AsyncService, erebus::AuthRequest, erebus::AuthReply> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createAuthorizeRpc, this);

        rpcHandlers.processIncomingRequest = [this](Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message) { ErebusService::processAuthorizeRpc(rpc, message); };
        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestAuthorize;

        new Er::Server::Private::Rpc::UnaryRpc<erebus::Erebus::AsyncService, erebus::AuthRequest, erebus::AuthReply>(&m_service, m_queue.get(), rpcHandlers);
    }

    void processAuthorizeRpc(Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
    {
        auto request = static_cast<const erebus::AuthRequest*>(message);

        erebus::AuthReply response;
        auto& user = request->user();
        auto u = m_params.userDb->lookup(user);
        if (!u)
        {
            Er::Log::Warning(m_params.log, ErLogInstance("ErebusService")) << "Trying to log in an unknown user " << user;
            marshalException(response.mutable_header(), Result::NotFound, "User not found");
        }
        else
        {
            if (request->pwd() != u->pwdHash)
            {
                Er::Log::Warning(m_params.log, ErLogInstance("ErebusService")) << "Failed to log in user " << user;
                marshalException(response.mutable_header(), Result::Unauthenticated, "Wrong password");
            }
            else
            {
                Er::Log::Info(m_params.log, ErLogInstance("ErebusService")) << "Logged in user " << user;

                auto ticket = makeTicket();
                response.set_ticket(ticket);

                m_authProcessor->addTicket(user, ticket);
            }
        }

        rpc.sendResponse(&response);
    }

    void createAddUserRpc()
    {
        Er::Server::Private::Rpc::UnaryRpcHandlers<erebus::Erebus::AsyncService, erebus::AddUserRequest, erebus::GenericReply> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createAddUserRpc, this);

        rpcHandlers.processIncomingRequest = [this](Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message) { ErebusService::processAddUserRpc(rpc, message); };
        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestAddUser;

        new Er::Server::Private::Rpc::UnaryRpc<erebus::Erebus::AsyncService, erebus::AddUserRequest, erebus::GenericReply>(&m_service, m_queue.get(), rpcHandlers);
    }

    void processAddUserRpc(Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
    {
        auto request = static_cast<const erebus::AddUserRequest*>(message);
        erebus::GenericReply response;

        if (!checkAuth(rpc))
            return;

        try
        {
            auto& name = request->name();
            m_params.userDb->add(Er::Server::Private::User(name, request->salt(), request->pwd()));
            m_params.userDb->save();

            Er::Log::Info(m_params.log, ErLogInstance("ErebusService")) << "Created user " << name;
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

    void createRemoveUserRpc()
    {
        Er::Server::Private::Rpc::UnaryRpcHandlers<erebus::Erebus::AsyncService, erebus::RemoveUserRequest, erebus::GenericReply> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createRemoveUserRpc, this);

        rpcHandlers.processIncomingRequest = [this](Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message) { ErebusService::processRemoveUserRpc(rpc, message); };
        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestRemoveUser;

        new Er::Server::Private::Rpc::UnaryRpc<erebus::Erebus::AsyncService, erebus::RemoveUserRequest, erebus::GenericReply>(&m_service, m_queue.get(), rpcHandlers);
    }

    void processRemoveUserRpc(Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
    {
        auto request = static_cast<const erebus::RemoveUserRequest*>(message);
        erebus::GenericReply response;

        if (!checkAuth(rpc))
            return;

        try
        {
            auto& name = request->name();
            m_params.userDb->remove(name);
            m_params.userDb->save();

            Er::Log::Info(m_params.log, ErLogInstance("ErebusService")) << "Deleted user " << name;
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

    void createListUsersRpc()
    {
        Er::Server::Private::Rpc::UnaryRpcHandlers<erebus::Erebus::AsyncService, erebus::Void, erebus::ListUsersReply> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createListUsersRpc, this);

        rpcHandlers.processIncomingRequest = [this](Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message) { ErebusService::processListUsersRpc(rpc, message); };
        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestListUsers;

        new Er::Server::Private::Rpc::UnaryRpc<erebus::Erebus::AsyncService, erebus::Void, erebus::ListUsersReply>(&m_service, m_queue.get(), rpcHandlers);
    }

    void processListUsersRpc(Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
    {
        auto request = static_cast<const erebus::Void*>(message);
        erebus::ListUsersReply response;

        if (!checkAuth(rpc))
            return;

        try
        {
            auto list = m_params.userDb->enumerate();
            ErAssert(!list.empty());

            auto users = response.mutable_users();
            users->Reserve(list.size());

            for (auto& user : list)
            {
                auto u = users->Add();
                u->set_name(user.name);
            }

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

    void createExitRpc()
    {
        Er::Server::Private::Rpc::UnaryRpcHandlers<erebus::Erebus::AsyncService, erebus::ExitRequest, erebus::GenericReply> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createExitRpc, this);

        rpcHandlers.processIncomingRequest = [this](Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message) { ErebusService::processExitRpc(rpc, message); };
        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestExit;

        new Er::Server::Private::Rpc::UnaryRpc<erebus::Erebus::AsyncService, erebus::ExitRequest, erebus::GenericReply>(&m_service, m_queue.get(), rpcHandlers);
    }

    void processExitRpc(Er::Server::Private::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
    {
        auto request = static_cast<const erebus::ExitRequest*>(message);
        erebus::GenericReply response;

        if (!checkAuth(rpc))
            return;

        *m_params.needRestart = request->restart();

        if (*m_params.needRestart)
            Er::Log::Warning(m_params.log, ErLogInstance("ErebusService")) << "Server restart requested by " << rpc.getServerContext().peer();
        else
            Er::Log::Warning(m_params.log, ErLogInstance("ErebusService")) << "Server shutdown requested by " << rpc.getServerContext().peer();

       
        rpc.sendResponse(&response);
        // give the gRPC a chance to send back a reply before we die
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        m_params.exitCondition->set(true);
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

        if (!checkAuth(rpc))
            return;

        std::shared_lock l(m_servicesLock);
        
        auto& serviceId = request->request();

        auto it = m_services.find(serviceId);
        if (it == m_services.end())
        {
            m_params.log->write(Er::Log::Level::Error, ErLogInstance("ErebusService"), "No handlers for [%s]", serviceId.c_str());
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

        if (!checkAuth(rpc))
            return;

        std::shared_lock l(m_servicesLock);
        
        auto& serviceId = request->request();

        auto it = m_services.find(serviceId);
        if (it == m_services.end())
        {
            m_params.log->write(Er::Log::Level::Error, ErLogInstance("ErebusService"), "No handlers for [%s]", serviceId.c_str());
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

        if (!checkAuth(rpc))
            return;

        auto& id = request->request();
        std::optional<uint32_t> sessionId;
        if (request->has_sessionid())
            sessionId = request->sessionid();

        if (sessionId)
            response.set_sessionid(*sessionId);

        std::shared_lock l(m_servicesLock);
        
        auto it = m_services.find(id);
        if (it == m_services.end())
        {
            m_params.log->write(Er::Log::Level::Error, ErLogInstance("ErebusService"), "No handlers for [%s]", id.c_str());
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
        
        if (!checkAuth(rpc))
            return;

        auto& id = request->request();
        std::optional<uint32_t> sessionId;
        if (request->has_sessionid())
            sessionId = request->sessionid();

        std::shared_lock l(m_servicesLock);
        
        auto it = m_services.find(id);
        if (it == m_services.end())
        {
            m_params.log->write(Er::Log::Level::Error, ErLogInstance("ErebusService"), "No handlers for [%s]", id.c_str());
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
                        response.set_sessionid(*sessionId);

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