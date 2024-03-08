#include <erebus/exception.hxx>
#include <erebus-processmgr/processmgr.hxx>
#include <erebus-processmgr/processprops.hxx>

#include "desktopentries.hxx"
#include "iconcache.hxx"
#include "iconmanager.hxx"
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
        m_iconManager.reset();
        m_desktopEntries.reset();
        m_iconCache.reset();

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

        // cache desktop entries & icons for registered apps
        if (!args.iconCacheAgent.empty())
            m_iconCache.reset(new Er::Private::IconCache(params.log, args.iconTheme, args.iconCacheAgent, args.iconCacheDir));
        else
            params.log->write(Er::Log::Level::Warning, LogComponent("ProcessMgrPlugin"), "Starting without icon cache");

        m_desktopEntries.reset(new Er::Private::DesktopEntries(params.log));

        if (m_iconCache)
        {
            m_iconManager.reset(new Er::Private::IconManager(params.log, m_iconCache.get(), m_desktopEntries.get(), args.iconCacheSize));
            m_iconManager->prefetch(Er::Private::IconSize::Small);
        }

        // create and register services
        m_processList.reset(new Er::Private::ProcessList(m_params.log, m_iconManager.get()));
        
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
        size_t iconCacheSize;
        std::string iconTheme;
    };

    PluginArgs parseArgs(const Er::Server::PluginParams& params)
    {
        PluginArgs a;

        for (auto arg = params.args.begin(); arg != params.args.end(); ++arg)
        {
            if ((*arg == "iconcacheagent") && (std::next(arg) != params.args.end()))
            {
                arg = std::next(arg);
                a.iconCacheAgent = *arg;
            }
            else if ((*arg == "iconcachedir") && (std::next(arg) != params.args.end()))
            {
                arg = std::next(arg);
                a.iconCacheDir = *arg;
            }
            else if ((*arg == "icontheme") && (std::next(arg) != params.args.end()))
            {
                arg = std::next(arg);
                a.iconTheme = *arg;
            }
            else if ((*arg == "iconcachesize") && (std::next(arg) != params.args.end()))
            {
                arg = std::next(arg);
                a.iconCacheSize = std::strtoul(arg->c_str(), nullptr, 10);
                if (a.iconCacheSize == 0)
                    a.iconCacheSize = 1024;
            }
        }

        return a;
    }

    static std::atomic<long> g_instances;

    Er::Server::PluginParams m_params;
    std::unique_ptr<Er::Private::IconCache> m_iconCache;
    std::unique_ptr<Er::Private::DesktopEntries> m_desktopEntries;
    std::unique_ptr<Er::Private::IconManager> m_iconManager;
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