#include <erebus/exception.hxx>
#include <erebus-processmgr/desktopentry.hxx>
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

        m_processList.reset();
        m_desktopEntries.reset();

        Er::ProcessProps::Private::unregisterAll();

        g_instances--;
    }

    explicit ProcessMgrPlugin(const Er::Server::PluginParams& params)
        : m_params(params)
    {
        long expected = 0;
        if (!g_instances.compare_exchange_strong(expected, 1, std::memory_order_acq_rel))
            throw Er::Exception(ER_HERE(), "Only one instance of erebus-processmgr plugin can be instantiated");

        auto args = parseArgs(params);
        m_desktopEntries.reset(new DesktopEnv::DesktopEntries(params.log, args.iconCacheAgent, args.iconCacheDir));

        // create and register services
        m_processList.reset(new Er::Private::ProcessList(m_params.log));
        
        for (auto container: m_params.containers)
        {
            container->registerService(Er::ProcessRequests::ListProcesses, m_processList.get());
            container->registerService(Er::ProcessRequests::ListProcessesDiff, m_processList.get());
            container->registerService(Er::ProcessRequests::ProcessDetails, m_processList.get());
        }

        Er::ProcessProps::Private::registerAll();
    }

private:
    struct PluginArgs
    {
        std::string iconCacheAgent;
        std::string iconCacheDir;
    };

    PluginArgs parseArgs(const Er::Server::PluginParams& params)
    {
        PluginArgs a;

        bool iconCacheAgentNext = false;
        bool iconCacheDirNext = false;
        for (auto& arg: params.args)
        {
            if (arg == "--iconcacheagent")
            {
                iconCacheAgentNext = true;
            }
            else if (iconCacheAgentNext)
            {
                iconCacheAgentNext = false;
                a.iconCacheAgent = arg;
            }
            else if (arg == "--iconcachedir")
            {
                iconCacheDirNext = true;
            }
            else if (iconCacheDirNext)
            {
                iconCacheDirNext = false;
                a.iconCacheDir = arg;
            }
        }

        return a;
    }

    static std::atomic<long> g_instances;

    Er::Server::PluginParams m_params;
    std::unique_ptr<DesktopEnv::DesktopEntries> m_desktopEntries;
    std::unique_ptr<Er::Private::ProcessList> m_processList;
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
    if (!plugin)
        return;

    auto realPlugin = dynamic_cast<Er::ProcessMgrPlugin*>(plugin);
    assert(realPlugin);
    delete realPlugin;
}

} // extern "C" {}