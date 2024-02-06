#include "erebus-version.h"

#include <erebus/exception.hxx>
#include <erebus/knownprops.hxx>
#include <erebus/util/format.hxx>
#include <erebus/util/random.hxx>
#include <erebus-srv/erebus-srv.hxx>
#include <erebus-srv/userdb.hxx>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include <erebus/erebus.grpc.pb.h>

#include <atomic>
#include <mutex>
#include <unordered_map>


namespace Er
{

namespace Private
{

namespace Server
{

namespace
{

class AuthMetadataProcessor 
    : public grpc::AuthMetadataProcessor
{
public:
    explicit AuthMetadataProcessor(Er::Log::ILog* log)
        : m_log(log)
    {
    }

    grpc::Status Process(const InputMetadata& authMetadata, grpc::AuthContext* context, OutputMetadata* consumedMetadata, OutputMetadata* responseMetadata) override
    {
        // determine intercepted method
        auto dispatch = authMetadata.find(":path");
        if (dispatch == authMetadata.end())
        {
            m_log->write(Er::Log::Level::Error, "No method path in metadata");
            return grpc::Status(grpc::StatusCode::INTERNAL, "Internal Error");
        }

        // if token metadata not necessary, return early, avoid token checking
        auto dispatchValue = std::string(dispatch->second.data(), dispatch->second.length());
        if (
            (dispatchValue == "/erebus.Erebus/Authorize") ||
            (dispatchValue == "/erebus.Erebus/Init") ||
            (dispatchValue == "/erebus.Erebus/Version")
            )
        {
            return grpc::Status::OK;
        }

        // determine availability of ticket metadata
        auto ticket = authMetadata.find("ticket");
        if (ticket == authMetadata.end())
        {
            m_log->write(Er::Log::Level::Error, "No ticket in metadata");
            return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Missing Ticket");
        }

        // determine validity of token metadata
        auto ticketValue = std::string(ticket->second.data(), ticket->second.length());
        
        std::lock_guard l(m_mutex);
        auto it = m_tickets.find(ticketValue);
        if (it == m_tickets.end())
        {
            m_log->write(Er::Log::Level::Error, "Invalid ticket");
            return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid Ticket");
        }

        // once verified, mark as consumed and store user for later retrieval
        consumedMetadata->insert(std::make_pair("ticket", ticketValue));     // required
        context->AddProperty("user", it->second);           // optional
        context->SetPeerIdentityPropertyName("user");                 // optional

        return grpc::Status::OK;
    }

    void addTicket(const std::string& user, const std::string& ticket)
    {
        std::lock_guard l(m_mutex);
        m_tickets.insert({ ticket, user });
    }

private:
    Er::Log::ILog* m_log;
    std::mutex m_mutex;
    std::unordered_map<std::string, std::string> m_tickets; // ticket -> user
};

class ErebusSrv final
    : public IServer
    , public erebus::Erebus::Service
{
public:
    ~ErebusSrv()
    {
    }

    explicit ErebusSrv(const Params* params)
        : m_params(*params)
        , m_local(params->endpoint.starts_with("unix:"))
        , m_authProcessor(std::make_shared<AuthMetadataProcessor>(params->log))
    {
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
        builder.RegisterService(this);

        // finally assemble the server
        auto server = builder.BuildAndStart();
        if (!server)
            throw Er::Exception(ER_HERE(), "Failed to start the server");

        m_server.swap(server);
    }

    void stop() override
    {
        m_server->Shutdown();
    }

    void wait() override
    {
        m_server->Wait();
    }

    grpc::Status Init(::grpc::ServerContext* context, const ::erebus::InitialRequest* request, ::erebus::InitialReply* response) override
    {
        auto& user = request->user();
        auto u = m_params.userDb->lookup(user);
        if (!u)
        {
            Er::Log::Warning(m_params.log) << "Trying to log in an unknown user " << user;
            response->mutable_header()->set_code(erebus::Unauthenticated);
        }
        else
        {
            response->mutable_header()->set_code(erebus::Success);
            response->set_salt(u->pwdSalt);
        }

        return grpc::Status::OK;
    }

    grpc::Status Authorize(::grpc::ServerContext* context, const ::erebus::AuthRequest* request, ::erebus::AuthReply* response) override
    {
        auto& user = request->user();
        auto u = m_params.userDb->lookup(user);
        if (!u)
        {
            Er::Log::Warning(m_params.log) << "Trying to log in an unknown user " << user;
            response->mutable_header()->set_code(erebus::Unauthenticated);
        }
        else
        {
            if (request->pwd() != u->pwdHash)
            {
                Er::Log::Warning(m_params.log) << "Failed to log in user " << user;
                response->mutable_header()->set_code(erebus::Unauthenticated);
            }
            else
            {
                Er::Log::Info(m_params.log) << "Logged in user " << user;
                response->mutable_header()->set_code(erebus::Success);

                auto ticket = makeTicket();
                response->set_ticket(ticket);

                m_authProcessor->addTicket(user, ticket);
            }
        }

        return grpc::Status::OK;
    }

    grpc::Status AddUser(::grpc::ServerContext* context, const ::erebus::AddUserRequest* request, ::erebus::GenericReply* response) override
    {
        if (!context->auth_context()->IsPeerAuthenticated())
            return grpc::Status(grpc::UNAUTHENTICATED, "Unauthenticated");

        try
        {
            auto& name = request->name();
            m_params.userDb->add(Er::Private::Server::User(name, request->salt(), request->pwd()));
            m_params.userDb->save();

            response->set_code(erebus::Success);
            Er::Log::Info(m_params.log) << "Created user " << name;
        }
        catch (Er::Exception& e)
        {
            response->set_code(erebus::Failure);
            marshalException(response, e);
        }
        catch (std::exception& e)
        {
            response->set_code(erebus::Failure);
            marshalException(response, e);
        }

        return grpc::Status::OK;
    }

    grpc::Status RemoveUser(::grpc::ServerContext* context, const ::erebus::RemoveUserRequest* request, ::erebus::GenericReply* response) override
    {
        if (!context->auth_context()->IsPeerAuthenticated())
            return grpc::Status(grpc::UNAUTHENTICATED, "Unauthenticated");

        try
        {
            auto& name = request->name();
            m_params.userDb->remove(name);
            m_params.userDb->save();

            response->set_code(erebus::Success);
            Er::Log::Info(m_params.log) << "Deleted user " << name;
        }
        catch (Er::Exception& e)
        {
            response->set_code(erebus::Failure);
            marshalException(response, e);
        }
        catch (std::exception& e)
        {
            response->set_code(erebus::Failure);
            marshalException(response, e);
        }

        return grpc::Status::OK;
    }

    grpc::Status ListUsers(::grpc::ServerContext* context, const ::erebus::Void* request, ::erebus::ListUsersReply* response) override
    {
        if (!context->auth_context()->IsPeerAuthenticated())
            return grpc::Status(grpc::UNAUTHENTICATED, "Unauthenticated");

        try
        {
            auto list = m_params.userDb->enumerate();
            assert(!list.empty());

            auto users = response->mutable_users();
            users->Reserve(list.size());

            for (auto& user : list)
            {
                auto u = users->Add();
                u->set_name(user.name);
            }
            
            response->mutable_header()->set_code(erebus::Success);
        }
        catch (Er::Exception& e)
        {
            response->mutable_header()->set_code(erebus::Failure);
            marshalException(response->mutable_header(), e);
        }
        catch (std::exception& e)
        {
            response->mutable_header()->set_code(erebus::Failure);
            marshalException(response->mutable_header(), e);
        }

        return grpc::Status::OK;
    }

    grpc::Status Exit(grpc::ServerContext* context, const erebus::ExitRequest* request, erebus::GenericReply* response) override
    {
        if (!context->auth_context()->IsPeerAuthenticated())
            return grpc::Status(grpc::UNAUTHENTICATED, "Unauthenticated");

        *m_params.needRestart = request->restart();

        if (*m_params.needRestart)
            Er::Log::Warning(m_params.log) << "Server restart requested by " << context->peer();
        else
            Er::Log::Warning(m_params.log) << "Server shutdown requested by " << context->peer();

        response->set_code(erebus::ResultCode::Success);
        m_params.exitCondition->set();
        
        return grpc::Status::OK;
    }

    grpc::Status Version(grpc::ServerContext* context, const erebus::Void* request, erebus::ServerVersionReply* response) override
    {
        response->mutable_header()->set_code(erebus::Success);
        response->set_major(ER_VERSION_MAJOR);
        response->set_minor(ER_VERSION_MINOR);
        response->set_patch(ER_VERSION_PATCH);

        return grpc::Status::OK;
    }
    
private:
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

            for (auto& property: *properties)
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

    Params m_params;
    bool m_local;
    std::unique_ptr<grpc::Server> m_server;
    std::shared_ptr<AuthMetadataProcessor> m_authProcessor;
};


} // namespace {}


static std::atomic<long> g_initialized = 0;

EREBUSSRV_EXPORT void initialize()
{
    if (g_initialized.fetch_add(1, std::memory_order_acq_rel) == 0)
    {
        ::grpc_init();

        grpc::EnableDefaultHealthCheckService(true);
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    }
}

EREBUSSRV_EXPORT void finalize()
{
    if (g_initialized.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        ::grpc_shutdown();
    }
}

std::shared_ptr<IServer> EREBUSSRV_EXPORT start(const Params* params)
{
    auto result = std::make_shared<ErebusSrv>(params);

    return result;
}

} // namespace Server {}

} // namespace Private {}

} // namespace Er {}