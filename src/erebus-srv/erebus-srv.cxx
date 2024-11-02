#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include <erebus/erebus.grpc.pb.h>
#include <erebus-srv/erebus-srv.hxx>
#include <erebus-srv/global_requests.hxx>


#include <atomic>


namespace Er
{

namespace Server
{

namespace
{

Er::Log::ILog* g_log = nullptr;
std::atomic<long> g_initialized = 0;


void gprLogFunction(gpr_log_func_args* args)
{
    if (g_log)
    {
        Er::Log::Level level = Er::Log::Level::Debug;
        switch (args->severity)
        {
        case GPR_LOG_SEVERITY_INFO: level = Log::Level::Info; break;
        case GPR_LOG_SEVERITY_ERROR: level = Log::Level::Error; break;
        }

        Er::Log::write(g_log, level, "[gRPC] {}", args->message);
    }
}

} // namespace {}


EREBUSSRV_EXPORT void initialize(Er::Log::ILog* log)
{
    if (g_initialized.fetch_add(1, std::memory_order_acq_rel) == 0)
    {
        Er::Server::Props::Private::registerAll(log);

        g_log = log;

        ::grpc_init();

        if (log->level() == Log::Level::Debug)
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

        Er::Server::Props::Private::unregisterAll(g_log);

        g_log = nullptr;
    }
}


} // namespace Server {}

} // namespace Er {}