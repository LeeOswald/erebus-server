#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include <erebus/erebus.grpc.pb.h>
#include <erebus-srv/erebus-srv.hxx>
#include <erebus/protocol.hxx>


#include <atomic>


namespace Er
{

namespace Server
{

namespace Private
{

namespace
{

LibParams g_libParams;
std::atomic<long> g_initialized = 0;


void gprLogFunction(gpr_log_func_args* args)
{
    if (g_libParams.log)
    {
        Er::Log::Level level = Er::Log::Level::Debug;
        switch (args->severity)
        {
        case GPR_LOG_SEVERITY_INFO: level = Log::Level::Info; break;
        case GPR_LOG_SEVERITY_ERROR: level = Log::Level::Error; break;
        }

        g_libParams.log->writef(level, "[gRPC] %s", args->message);
    }
}

} // namespace {}


EREBUSSRV_EXPORT void initialize(const LibParams& params)
{
    if (g_initialized.fetch_add(1, std::memory_order_acq_rel) == 0)
    {
        Er::Protocol::Props::Private::registerAll(params.log);

        g_libParams = params;

        ::grpc_init();

        if (params.level == Log::Level::Debug)
            ::gpr_set_log_verbosity(GPR_LOG_SEVERITY_DEBUG);
        else
            ::gpr_set_log_verbosity(GPR_LOG_SEVERITY_INFO);

        ::gpr_set_log_function(gprLogFunction);

        grpc::EnableDefaultHealthCheckService(true);
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    }
}

EREBUSSRV_EXPORT void finalize()
{
    if (g_initialized.fetch_sub(1, std::memory_order_acq_rel) == 1)
    {
        ::grpc_shutdown();

        Er::Protocol::Props::Private::unregisterAll(g_libParams.log);

        g_libParams = LibParams();
    }
}

} // namespace Private {}

} // namespace Server {}

} // namespace Er {}