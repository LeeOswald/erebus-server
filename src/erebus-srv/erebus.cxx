#include "erebus-version.h"
#include "svcbase.hxx"

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
    , public ServiceBase
{
public:
    ~ErebusService() 
    {
        m_params.log->write(Log::Level::Debug, "ErebusService %p destroyed", this);
    }

    explicit ErebusService(const Params* params)
        : ServiceBase(params)
    {
        m_authProcessor->addNoAuthMethod("/erebus.Erebus/Init");
        m_authProcessor->addNoAuthMethod("/erebus.Erebus/Authorize");

        m_params.log->write(Log::Level::Debug, "ErebusService %p created", this);
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
        response.mutable_header()->set_code(erebus::Success);
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
        response.mutable_header()->set_code(erebus::Success);
     
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
            Er::Log::Warning(m_params.log) << "Trying to log in an unknown user " << user;
            response.mutable_header()->set_code(erebus::Unauthenticated);
        }
        else
        {
            response.mutable_header()->set_code(erebus::Success);
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
            Er::Log::Warning(m_params.log) << "Trying to log in an unknown user " << user;
            response.mutable_header()->set_code(erebus::Unauthenticated);
        }
        else
        {
            if (request->pwd() != u->pwdHash)
            {
                Er::Log::Warning(m_params.log) << "Failed to log in user " << user;
                response.mutable_header()->set_code(erebus::Unauthenticated);
            }
            else
            {
                Er::Log::Info(m_params.log) << "Logged in user " << user;
                response.mutable_header()->set_code(erebus::Success);

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

            response.set_code(erebus::Success);
            Er::Log::Info(m_params.log) << "Created user " << name;
        }
        catch (Er::Exception& e)
        {
            response.set_code(erebus::Failure);
            marshalException(&response, e);
        }
        catch (std::exception& e)
        {
            response.set_code(erebus::Failure);
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

            response.set_code(erebus::Success);
            Er::Log::Info(m_params.log) << "Deleted user " << name;
        }
        catch (Er::Exception& e)
        {
            response.set_code(erebus::Failure);
            marshalException(&response, e);
        }
        catch (std::exception& e)
        {
            response.set_code(erebus::Failure);
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
            assert(!list.empty());

            auto users = response.mutable_users();
            users->Reserve(list.size());

            for (auto& user : list)
            {
                auto u = users->Add();
                u->set_name(user.name);
            }

            response.mutable_header()->set_code(erebus::Success);
        }
        catch (Er::Exception& e)
        {
            response.mutable_header()->set_code(erebus::Failure);
            marshalException(response.mutable_header(), e);
        }
        catch (std::exception& e)
        {
            response.mutable_header()->set_code(erebus::Failure);
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
            Er::Log::Warning(m_params.log) << "Server restart requested by " << rpc.getServerContext().peer();
        else
            Er::Log::Warning(m_params.log) << "Server shutdown requested by " << rpc.getServerContext().peer();

        response.set_code(erebus::ResultCode::Success);
        
        rpc.sendResponse(&response);
        // give the gRPC a chance to send back a reply before we die
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        m_params.exitCondition->set();
    }

    erebus::Erebus::AsyncService m_service;
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