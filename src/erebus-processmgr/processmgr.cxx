#include <erebus/exception.hxx>
#include <erebus-processmgr/processmgr.hxx>
#include <erebus-processmgr/processprops.hxx>

#include "processlist.hxx"

#include <atomic>

namespace Er
{

namespace
{

class ProcessMgrPlugin final
    : public Er::Server::IPlugin
    , public Er::NonCopyable
{
public:
    ~ProcessMgrPlugin()
    {
        // unregister services
        for (auto container: m_params.containers)
        {
            container->unregisterService(m_processList.get());
        }

        Er::ProcessProps::Private::unregisterAll();

        g_instances--;
    }

    explicit ProcessMgrPlugin(const Er::Server::PluginParams& params)
        : m_params(params)
    {
        long expected = 0;
        if (!g_instances.compare_exchange_strong(expected, 1, std::memory_order_acq_rel))
            throw Er::Exception(ER_HERE(), "Only one instance of erebus-processmgr plugin can be instantiated");

        // create and register services
        m_processList = std::make_shared<Er::Private::ProcessList>(m_params.log);
        for (auto container: m_params.containers)
        {
            container->registerService(Er::ProcessRequests::ListProcesses, m_processList.get());
            container->registerService(Er::ProcessRequests::ProcessDetails, m_processList.get());
        }

        Er::ProcessProps::Private::registerAll();
    }

private:
    static std::atomic<long> g_instances;

    Er::Server::PluginParams m_params;
    std::shared_ptr<Er::Private::ProcessList> m_processList;
};

std::atomic<long> ProcessMgrPlugin::g_instances = 0;


} // namespace {}

} // namespace Er {}


extern "C"
{

ER_PROCESSMGR_EXPORT Er::Server::IPlugin* createPlugin(const Er::Server::PluginParams& params)
{
    return new Er::ProcessMgrPlugin(params);
}

ER_PROCESSMGR_EXPORT void disposePlugin(Er::Server::IPlugin* plugin)
{
    delete plugin;
}

} // extern "C" {}