#include "erebus-version.h"

#include <erebus/exception.hxx>
#include <erebus-srv/erebus-srv.hxx>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include <protocol/erebus.grpc.pb.h>

#include <atomic>


namespace Er
{

namespace Private
{

namespace Server
{

namespace
{

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
    {
        auto local = params->endpoint.starts_with("unix:");
        grpc::ServerBuilder builder;

        if (!local && !params->certificate.empty())
        {
            grpc::SslServerCredentialsOptions::PemKeyCertPair keycert = { params->key, params->certificate };
            grpc::SslServerCredentialsOptions sslOps;
            sslOps.pem_root_certs = params->root;
            sslOps.pem_key_cert_pairs.push_back(keycert);

            builder.AddListeningPort(params->endpoint, grpc::SslServerCredentials(sslOps));
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
        response->set_major(ER_VERSION_MAJOR);
        response->set_minor(ER_VERSION_MINOR);
        response->set_patch(ER_VERSION_PATCH);

        return grpc::Status::OK;
    }
    
private:
    Params m_params;
    std::unique_ptr<grpc::Server> m_server;
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