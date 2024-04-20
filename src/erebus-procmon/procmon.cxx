#include <erebus/exception.hxx>
#include <erebus/log.hxx>

#include "process_spy.hxx"

#include <atomic>

namespace Er
{
    
namespace
{
    

class ProcMonPlugin final
    : public Er::Server::IPlugin
    , public Er::NonCopyable
{
public:
    ~ProcMonPlugin()
    {
        m_processSpy.reset();

        g_instances--;
        ::libbpf_set_print(nullptr);
        g_log = nullptr;
    }

    explicit ProcMonPlugin(const Er::Server::PluginParams& params)
        : m_params(params)
    {
        long expected = 0;
        if (!g_instances.compare_exchange_strong(expected, 1, std::memory_order_acq_rel))
            throw Er::Exception(ER_HERE(), "Only one instance of erebus-procmon plugin can be created");

        g_log = params.log;
        ::libbpf_set_print(libbpf_print_fn);

        ::libbpf_set_strict_mode(libbpf_strict_mode(LIBBPF_STRICT_CLEAN_PTRS | LIBBPF_STRICT_DIRECT_ERRS));

        m_processSpy.reset(new Er::Private::ProcessSpy(m_params.log));
    }

private:
    static int libbpf_print_fn(enum libbpf_print_level level, const char* format, va_list args) noexcept
    {
        if (!g_log) [[unlikely]]
            return 0;

        Er::Log::Level l = Er::Log::Level::Info;
        if (level == LIBBPF_DEBUG)
            l = Er::Log::Level::Debug;
        else if (level == LIBBPF_WARN)
            l = Er::Log::Level::Warning;

        g_log->writev(l, LogComponent("eBPF"), format, args);

        return 0;
    }


    static std::atomic<long> g_instances;
    static Er::Log::ILog* g_log;

    Er::Server::PluginParams m_params;
    std::unique_ptr<Er::Private::ProcessSpy> m_processSpy;
};

std::atomic<long> ProcMonPlugin::g_instances = 0;
Er::Log::ILog* ProcMonPlugin::g_log = nullptr;
    
} // namespace {}
    
} // namespace Er {}



extern "C"
{

ER_PROCMON_EXPORT Er::Server::IPlugin* createPlugin(const Er::Server::PluginParams& params)
{
    return new Er::ProcMonPlugin(params);
}

ER_PROCMON_EXPORT void disposePlugin(Er::Server::IPlugin* plugin)
{
    if (!plugin)
        return;

    auto realPlugin = dynamic_cast<Er::ProcMonPlugin*>(plugin);
    assert(realPlugin);
    delete realPlugin;
}

} // extern "C" {}