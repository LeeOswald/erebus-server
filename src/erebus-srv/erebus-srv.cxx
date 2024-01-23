#include "erebus-version.h"

#include <erebus/exception.hxx>
#include <erebus-srv/erebus-srv.hxx>

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include <protocol/erebus.grpc.pb.h>


namespace Er
{

namespace Private
{

namespace
{

class ErebusSrv final
    : public IErebusSrv
    , public erebus::Erebus::Service
{
public:
    ~ErebusSrv()
    {
    }

    explicit ErebusSrv(const ServerParams* params)
        : m_params(*params)
    {
        grpc::ServerBuilder builder;
        
        // listen on the given address without any authentication mechanism
        builder.AddListeningPort(params->address, grpc::InsecureServerCredentials());
        
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
    ServerParams m_params;
    std::unique_ptr<grpc::Server> m_server;
};


} // namespace {}


std::shared_ptr<IErebusSrv> EREBUSSRV_EXPORT startServer(const ServerParams* params)
{
    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    auto result = std::make_shared<ErebusSrv>(params);

    return result;
}

} // namespace Private {}

} // namespace Er {}