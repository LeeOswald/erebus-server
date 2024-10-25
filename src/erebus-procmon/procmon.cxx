#include <erebus/exception.hxx>
#include <erebus/log.hxx>
#include <erebus/util/cformat.hxx>

#include "process_spy.hxx"

#include "erebus-version.h"

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
        ::libbpf_set_print(m_oldBpfPrint);
        g_log = nullptr;
    }

    void dispose() noexcept override
    {
        delete this;
    }

    Er::Server::IPlugin::Info info() const override
    {
        return Er::Server::IPlugin::Info(
            "Process Monitor",
            "Process & thread events",
            Er::Server::IPlugin::Info::Version(ER_VERSION_MAJOR, ER_VERSION_MINOR, ER_VERSION_PATCH)
        );
    }

    explicit ProcMonPlugin(const Er::Server::PluginParams& params)
        : m_params(params)
    {
        long expected = 0;
        if (!g_instances.compare_exchange_strong(expected, 1, std::memory_order_acq_rel))
            ErThrow("Only one instance of erebus-procmon plugin can be created");

        g_log = params.log;
        m_oldBpfPrint = ::libbpf_set_print(libbpf_print_fn);

        ::libbpf_set_strict_mode(libbpf_strict_mode(LIBBPF_STRICT_CLEAN_PTRS | LIBBPF_STRICT_DIRECT_ERRS));

        m_processSpy.reset(new Erp::Procmon::ProcessSpy(m_params.log));
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

        std::string formatted;
        try
        {
            formatted = Er::Util::cformatv(format, args);
            if ((formatted.length() > 0) && (formatted[formatted.length() - 1] == '\n'))
                formatted.resize(formatted.length() - 1);
        
            Er::Log::writeln(g_log, l, std::move(formatted));
        }
        catch (std::exception&)
        {
        }
        
        return 0;
    }


    static std::atomic<long> g_instances;
    static Er::Log::ILog* g_log;

    Er::Server::PluginParams m_params;
    std::unique_ptr<Erp::Procmon::ProcessSpy> m_processSpy;
    libbpf_print_fn_t m_oldBpfPrint = nullptr;
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

} // extern "C" {}