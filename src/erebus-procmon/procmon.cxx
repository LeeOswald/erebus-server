#include <erebus/exception.hxx>

#include "procmon.hxx"


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
        g_instances--;
    }

    explicit ProcMonPlugin(const Er::Server::PluginParams& params)
        : m_params(params)
    {
        long expected = 0;
        if (!g_instances.compare_exchange_strong(expected, 1, std::memory_order_acq_rel))
            throw Er::Exception(ER_HERE(), "Only one instance of erebus-procmon plugin can be created");

    }

private:
    static std::atomic<long> g_instances;

    Er::Server::PluginParams m_params;
};

std::atomic<long> ProcMonPlugin::g_instances = 0;

    
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