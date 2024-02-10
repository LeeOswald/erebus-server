#include "erebus-version.h"

#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>
#include <erebus/util/random.hxx>
#include <erebus-srv/auth.hxx>
#include <erebus-srv/erebus-srv.hxx>
#include <erebus-srv/rpc.hxx>
#include <erebus-srv/userdb.hxx>

#include <erebus/erebus.grpc.pb.h>

#include <condition_variable>
#include <mutex>
#include <thread>

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
{
public:
    ~ErebusService() 
    {
        m_stop = true;
        m_incoming.notify_one();

        m_server->Shutdown();
        m_queue->Shutdown();

        // drain the CQ
        void* ignoredTag = nullptr;
        bool ignoredOk = false;
        while (m_queue->Next(&ignoredTag, &ignoredOk)) 
        {
        }

        if (m_receiverWorker->joinable())
            m_receiverWorker->join();

        if (m_processorWorker->joinable())
            m_processorWorker->join();

        m_params.log->write(Log::Level::Debug, "ErebusService %p destroyed", this);
    }

    explicit ErebusService(const Params* params)
        : m_params(*params)
        , m_local(params->endpoint.starts_with("unix:"))
        , m_authProcessor(std::make_shared<AuthMetadataProcessor>(params->log))
    {
        m_authProcessor->addNoAuthMethod("/erebus.Erebus/Init");
        m_authProcessor->addNoAuthMethod("/erebus.Erebus/Authorize");

        grpc::ServerBuilder builder;

        if (!m_local && params->ssl)
        {
            grpc::SslServerCredentialsOptions::PemKeyCertPair keycert = { params->key, params->certificate };
            grpc::SslServerCredentialsOptions sslOps;
            sslOps.pem_root_certs = params->root;
            sslOps.pem_key_cert_pairs.push_back(keycert);
            auto creds = grpc::SslServerCredentials(sslOps);
            creds->SetAuthMetadataProcessor(m_authProcessor);
            builder.AddListeningPort(params->endpoint, creds);
        }
        else
        {
            // listen on the given address without any authentication mechanism
            builder.AddListeningPort(params->endpoint, grpc::InsecureServerCredentials());
        }

        // register "service" as the instance through which we'll communicate with
        // clients. In this case it corresponds to an *synchronous* service
        builder.RegisterService(&m_service);
        m_queue = builder.AddCompletionQueue();

        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 1 * 60 * 1000);
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 20 * 1000);
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
        builder.AddChannelArgument(GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS, 10 * 1000);
        builder.AddChannelArgument(GRPC_ARG_HTTP2_MAX_PING_STRIKES, 5);

        // finally assemble the server
        auto server = builder.BuildAndStart();
        if (!server)
            throw Er::Exception(ER_HERE(), "Failed to start the server");

        m_server.swap(server);

        m_receiverWorker.reset(new std::thread([this]() { handleRpcs(); }));
        m_processorWorker.reset(new std::thread([this]() { processRpcs(); }));

        m_params.log->write(Log::Level::Debug, "ErebusService %p created", this);
    }

private:
    void handleRpcs()
    {
        Er::Log::Debug(m_params.log) << "RPC handler thread started";

        createDisconnectRpc();
        createVersionRpc();
        createInitRpc();
        createAuthorizeRpc();
        createAddUserRpc();
        createRemoveUserRpc();
        createListUsersRpc();
        createExitRpc();

        Er::Server::Rpc::TagInfo tagInfo;
        while (!m_stop)
        {
            // Block waiting to read the next event from the completion queue. The
            // event is uniquely identified by its tag, which in this case is the
            // memory address of a CallData instance.
            // The return value of Next should always be checked. This return value
            // tells us whether there is any kind of event or cq_ is shutting down.
            if (!m_queue->Next((void**)&tagInfo.tagProcessor, &tagInfo.ok))
            {
                if (!m_stop)
                {
                    m_params.log->write(Er::Log::Level::Warning, "No more tags in completion queue");
                    break;
                }
            }

            {
                std::lock_guard l(m_mutex);
                m_incomingTags.push_back(tagInfo);
            }

            m_incoming.notify_one();
        }

        Er::Log::Debug(m_params.log) << "RPC handler thread exited";
    }

    void processRpcs()
    {
        Er::Log::Debug(m_params.log) << "RPC processor thread started";

        while (!m_stop)
        {
            std::unique_lock l(m_mutex);

            m_incoming.wait(l, [this]() { return m_stop || !m_incomingTags.empty(); });

            if (m_stop)
                break;

            auto tags = std::move(m_incomingTags);
            l.unlock();

            while (!tags.empty())
            {
                auto tagInfo = tags.front();
                tags.pop_front();
                (*(tagInfo.tagProcessor))(tagInfo.ok);

            }
        }

        Er::Log::Debug(m_params.log) << "RPC processor thread exited";
    }

    static void genericDone(Er::Server::Rpc::RpcBase& rpc, bool rpcCancelled)
    {
        delete (&rpc);
    }

    void createVersionRpc()
    {
        Er::Server::Rpc::UnaryRpcHandlers<erebus::Erebus::AsyncService, erebus::Void, erebus::ServerVersionReply> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createVersionRpc, this);

        rpcHandlers.processIncomingRequest = [this](Er::Server::Rpc::RpcBase& rpc, const google::protobuf::Message* message) { ErebusService::processVersionRpc(rpc, message); };
        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestVersion;

        new Er::Server::Rpc::UnaryRpc<erebus::Erebus::AsyncService, erebus::Void, erebus::ServerVersionReply>(&m_service, m_queue.get(), rpcHandlers);
    }

    void processVersionRpc(Er::Server::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
    {
        if (!rpc.getServerContext().auth_context()->IsPeerAuthenticated())
        {
            rpc.finishWithError(grpc::Status(grpc::UNAUTHENTICATED, "Unauthenticated"));
            return;
        }

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
        Er::Server::Rpc::UnaryRpcHandlers<erebus::Erebus::AsyncService, erebus::Void, erebus::Void> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createDisconnectRpc, this);

        rpcHandlers.processIncomingRequest = [this](Er::Server::Rpc::RpcBase& rpc, const google::protobuf::Message* message) { ErebusService::processDisconnectRpc(rpc, message); };
        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestDisconnect;

        new Er::Server::Rpc::UnaryRpc<erebus::Erebus::AsyncService, erebus::Void, erebus::Void>(&m_service, m_queue.get(), rpcHandlers);
    }

    void processDisconnectRpc(Er::Server::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
    {
        if (!rpc.getServerContext().auth_context()->IsPeerAuthenticated())
        {
            rpc.finishWithError(grpc::Status(grpc::UNAUTHENTICATED, "Unauthenticated"));
            return;
        }

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
        Er::Server::Rpc::UnaryRpcHandlers<erebus::Erebus::AsyncService, erebus::InitialRequest, erebus::InitialReply> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createInitRpc, this);

        rpcHandlers.processIncomingRequest = [this](Er::Server::Rpc::RpcBase& rpc, const google::protobuf::Message* message) { ErebusService::processInitRpc(rpc, message); };
        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestInit;

        new Er::Server::Rpc::UnaryRpc<erebus::Erebus::AsyncService, erebus::InitialRequest, erebus::InitialReply>(&m_service, m_queue.get(), rpcHandlers);
    }

    void processInitRpc(Er::Server::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
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
        Er::Server::Rpc::UnaryRpcHandlers<erebus::Erebus::AsyncService, erebus::AuthRequest, erebus::AuthReply> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createAuthorizeRpc, this);

        rpcHandlers.processIncomingRequest = [this](Er::Server::Rpc::RpcBase& rpc, const google::protobuf::Message* message) { ErebusService::processAuthorizeRpc(rpc, message); };
        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestAuthorize;

        new Er::Server::Rpc::UnaryRpc<erebus::Erebus::AsyncService, erebus::AuthRequest, erebus::AuthReply>(&m_service, m_queue.get(), rpcHandlers);
    }

    void processAuthorizeRpc(Er::Server::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
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
        Er::Server::Rpc::UnaryRpcHandlers<erebus::Erebus::AsyncService, erebus::AddUserRequest, erebus::GenericReply> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createAddUserRpc, this);

        rpcHandlers.processIncomingRequest = [this](Er::Server::Rpc::RpcBase& rpc, const google::protobuf::Message* message) { ErebusService::processAddUserRpc(rpc, message); };
        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestAddUser;

        new Er::Server::Rpc::UnaryRpc<erebus::Erebus::AsyncService, erebus::AddUserRequest, erebus::GenericReply>(&m_service, m_queue.get(), rpcHandlers);
    }

    void processAddUserRpc(Er::Server::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
    {
        auto request = static_cast<const erebus::AddUserRequest*>(message);
        erebus::GenericReply response;

        if (!rpc.getServerContext().auth_context()->IsPeerAuthenticated())
        {
            rpc.finishWithError(grpc::Status(grpc::UNAUTHENTICATED, "Unauthenticated"));
            return;
        }

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
        Er::Server::Rpc::UnaryRpcHandlers<erebus::Erebus::AsyncService, erebus::RemoveUserRequest, erebus::GenericReply> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createRemoveUserRpc, this);

        rpcHandlers.processIncomingRequest = [this](Er::Server::Rpc::RpcBase& rpc, const google::protobuf::Message* message) { ErebusService::processRemoveUserRpc(rpc, message); };
        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestRemoveUser;

        new Er::Server::Rpc::UnaryRpc<erebus::Erebus::AsyncService, erebus::RemoveUserRequest, erebus::GenericReply>(&m_service, m_queue.get(), rpcHandlers);
    }

    void processRemoveUserRpc(Er::Server::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
    {
        auto request = static_cast<const erebus::RemoveUserRequest*>(message);
        erebus::GenericReply response;

        if (!rpc.getServerContext().auth_context()->IsPeerAuthenticated())
        {
            rpc.finishWithError(grpc::Status(grpc::UNAUTHENTICATED, "Unauthenticated"));
            return;
        }

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
        Er::Server::Rpc::UnaryRpcHandlers<erebus::Erebus::AsyncService, erebus::Void, erebus::ListUsersReply> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createListUsersRpc, this);

        rpcHandlers.processIncomingRequest = [this](Er::Server::Rpc::RpcBase& rpc, const google::protobuf::Message* message) { ErebusService::processListUsersRpc(rpc, message); };
        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestListUsers;

        new Er::Server::Rpc::UnaryRpc<erebus::Erebus::AsyncService, erebus::Void, erebus::ListUsersReply>(&m_service, m_queue.get(), rpcHandlers);
    }

    void processListUsersRpc(Er::Server::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
    {
        auto request = static_cast<const erebus::Void*>(message);
        erebus::ListUsersReply response;

        if (!rpc.getServerContext().auth_context()->IsPeerAuthenticated())
        {
            rpc.finishWithError(grpc::Status(grpc::UNAUTHENTICATED, "Unauthenticated"));
            return;
        }

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
        Er::Server::Rpc::UnaryRpcHandlers<erebus::Erebus::AsyncService, erebus::ExitRequest, erebus::GenericReply> rpcHandlers;

        rpcHandlers.createRpc = std::bind(&ErebusService::createExitRpc, this);

        rpcHandlers.processIncomingRequest = [this](Er::Server::Rpc::RpcBase& rpc, const google::protobuf::Message* message) { ErebusService::processExitRpc(rpc, message); };
        rpcHandlers.done = &genericDone;

        rpcHandlers.requestRpc = &erebus::Erebus::AsyncService::RequestExit;

        new Er::Server::Rpc::UnaryRpc<erebus::Erebus::AsyncService, erebus::ExitRequest, erebus::GenericReply>(&m_service, m_queue.get(), rpcHandlers);
    }

    void processExitRpc(Er::Server::Rpc::RpcBase& rpc, const google::protobuf::Message* message)
    {
        auto request = static_cast<const erebus::ExitRequest*>(message);
        erebus::GenericReply response;

        if (!rpc.getServerContext().auth_context()->IsPeerAuthenticated())
        {
            rpc.finishWithError(grpc::Status(grpc::UNAUTHENTICATED, "Unauthenticated"));
            return;
        }

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

    std::string getContextUserMapping(grpc::ServerContext* context) const
    {
        return context->auth_context()->GetPeerIdentity()[0].data();
    }

    std::string makeTicket() const
    {
        Er::Util::Random r;
        return r.generate(kTicketLength, kTicketChars);
    }

    void marshalException(erebus::GenericReply* reply, const std::exception& e)
    {
        auto what = e.what();
        if (!what || !*what)
            what = "Unknown exception";

        auto exception = reply->mutable_exception();
        *exception->mutable_message() = std::string_view(what);
    }

    void marshalException(erebus::GenericReply* reply, const Er::Exception& e)
    {
        std::string_view what;
        auto msg = e.message();
        if (msg)
            what = *msg;
        else
            what = "Unknown exception";

        auto exception = reply->mutable_exception();
        *exception->mutable_message() = what;

        auto location = e.location();
        if (location)
        {
            if (location->source)
            {
                exception->mutable_source()->set_file(location->source->file());
                exception->mutable_source()->set_line(location->source->line());
            }

            DecodedStackTrace ds;
            const DecodedStackTrace* stack = nullptr;
            if (location->decoded)
            {
                stack = &(*location->decoded);
            }
            else if (location->stack)
            {
                ds = decodeStackTrace(*location->stack);
                stack = &ds;
            }

            if (stack && !stack->empty())
            {
                auto mutableStack = exception->mutable_stack();
                for (auto& frame : *stack)
                {
                    if (!frame.empty())
                        mutableStack->add_frames(frame.c_str());
                    else
                        mutableStack->add_frames("???");
                }
            }
        }

        auto properties = e.properties();
        if (properties && !properties->empty())
        {
            auto mutableProps = exception->mutable_props();
            mutableProps->Reserve(properties->size());

            for (auto& property : *properties)
            {
                auto mutableProp = mutableProps->Add();
                mutableProp->set_id(property.id);

                auto info = property.info;
                if (!info)
                {
                    info = Er::lookupProperty(property.id).get();
                    assert(info);
                }

                auto& type = info->type();
                if (type == typeid(bool))
                    mutableProp->set_v_bool(std::any_cast<bool>(property.value));
                else if (type == typeid(int32_t))
                    mutableProp->set_v_int32(std::any_cast<int32_t>(property.value));
                else if (type == typeid(uint32_t))
                    mutableProp->set_v_uint32(std::any_cast<uint32_t>(property.value));
                else if (type == typeid(int64_t))
                    mutableProp->set_v_int64(std::any_cast<int64_t>(property.value));
                else if (type == typeid(uint64_t))
                    mutableProp->set_v_uint64(std::any_cast<uint64_t>(property.value));
                if (type == typeid(double))
                    mutableProp->set_v_double(std::any_cast<double>(property.value));
                if (type == typeid(std::string))
                    mutableProp->set_v_string(std::any_cast<std::string>(property.value));
                else
                    assert(!"unsupported property type");
            }

        }
    }


    const size_t kTicketLength = 64;
    const std::string_view kTicketChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";

    bool m_stop = false;
    Params m_params;
    bool m_local;
    std::shared_ptr<AuthMetadataProcessor> m_authProcessor;
    std::unique_ptr<grpc::ServerCompletionQueue> m_queue;
    erebus::Erebus::AsyncService m_service;
    std::unique_ptr<grpc::Server> m_server;
    std::mutex m_mutex;
    std::condition_variable m_incoming;
    Er::Server::Rpc::TagList m_incomingTags;
    std::unique_ptr<std::thread> m_receiverWorker;
    std::unique_ptr<std::thread> m_processorWorker;
};


} // namespace {}


std::shared_ptr<IServer> EREBUSSRV_EXPORT create(const Params* params)
{
    auto result = std::make_shared<ErebusService>(params);

    return result;
}

} // namespace Private {}

} // namespace Server {}

} // namespace Er {}