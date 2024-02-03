#include "erebus-version.h"

#include <erebus/exception.hxx>
#include <erebus/util/format.hxx>
#include <erebus/util/random.hxx>
#include <erebus-srv/erebus-srv.hxx>
#include <erebus-srv/userdb.hxx>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include <protocol/erebus.grpc.pb.h>

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
            (dispatchValue == "/erebus.Erebus/Init")
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
        //context->AddProperty("user", m_tokens[tokenValue]);           // optional
        //context->SetPeerIdentityPropertyName("user");                 // optional

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
            response->mutable_header()->set_code(erebus::NotFound);
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
            response->mutable_header()->set_code(erebus::NotFound);
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
        }
        catch (std::exception& e)
        {
            response->set_code(erebus::Failure);
        }

        return grpc::Status::OK;
    }

    grpc::Status Exit(grpc::ServerContext* context, const erebus::ExitRequest* request, erebus::GenericReply* response) override
    {
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
    void getContextUserMapping(grpc::ServerContext* context, std::string& username)
    {
        username = context->auth_context()->GetPeerIdentity()[0].data();
    }

    std::string makeTicket() const
    {
        Er::Util::Random r;
        return r.generate(kTicketLength, kTicketChars);
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