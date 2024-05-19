#include <erebus/exception.hxx>
#include <erebus-processmgr/processmgr.hxx>
#include <erebus-processmgr/processprops.hxx>

#include "desktopentries.hxx"
#include "iconcache.hxx"
#include "iconmanager.hxx"
#include "processdetails.hxx"
#include "processlist.hxx"

#include "erebus-version.h"

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
            m_processList->unregisterService(container);
            m_processDetails->unregisterService(container);
        }

        m_processList.reset();
        m_processDetails.reset();
        m_iconManager.reset();
        m_desktopEntries.reset();
        m_iconCache.reset();

        Er::ProcessProps::Private::unregisterAll(m_params.log);
        Er::ProcessesGlobal::Private::unregisterAll(m_params.log);

        g_instances--;
    }

    Er::Server::IPlugin::Info info() const override
    {
        return Er::Server::IPlugin::Info(
            "Process Tree",
            "Process tree and properties",
            Er::Server::IPlugin::Info::Version(ER_VERSION_MAJOR, ER_VERSION_MINOR, ER_VERSION_PATCH)
        );
    }

    explicit ProcessMgrPlugin(const Er::Server::PluginParams& params)
        : m_params(params)
    {
        long expected = 0;
        if (!g_instances.compare_exchange_strong(expected, 1, std::memory_order_acq_rel))
            throw Er::Exception(ER_HERE(), "Only one instance of erebus-processmgr plugin can be created");

        auto args = parseArgs(params);

        // cache desktop entries & icons for registered apps
        if (!args.iconCacheAgent.empty())
            m_iconCache.reset(new Er::Private::IconCache(params.log, args.iconTheme, args.iconCacheAgent, args.iconCacheDir));
        else
            params.log->write(Er::Log::Level::Warning, ErLogComponent("ProcessMgrPlugin"), "Starting without icon cache");

        m_desktopEntries.reset(new Er::Private::DesktopEntries(params.log));

        if (m_iconCache)
        {
            m_iconManager.reset(new Er::Private::IconManager(params.log, m_iconCache.get(), m_desktopEntries.get(), args.iconCacheSize));
            m_iconManager->prefetch(Er::Private::IconSize::Small);
        }

        // create and register services
        m_processList.reset(new Er::Private::ProcessList(m_params.log, m_iconManager.get()));
        m_processDetails.reset(new Er::Private::ProcessDetails(m_params.log));
        
        for (auto container: m_params.containers)
        {
            m_processList->registerService(container);
            m_processDetails->registerService(container);
        }

        Er::ProcessesGlobal::Private::registerAll(m_params.log);
        Er::ProcessProps::Private::registerAll(m_params.log);
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
            if (arg->name == "iconcacheagent")
            {
                a.iconCacheAgent = arg->value;
            }
            else if (arg->name == "iconcachedir")
            {
                a.iconCacheDir = arg->value;
            }
            else if (arg->name == "icontheme")
            {
                a.iconTheme = arg->value;
            }
            else if (arg->name == "iconcachesize")
            {
                a.iconCacheSize = std::strtoul(arg->value.c_str(), nullptr, 10);
            }
        }

        a.iconCacheSize = std::clamp(a.iconCacheSize, size_t(1024), size_t(65536));
        return a;
    }

    static std::atomic<long> g_instances;

    Er::Server::PluginParams m_params;
    std::unique_ptr<Er::Private::IconCache> m_iconCache;
    std::unique_ptr<Er::Private::DesktopEntries> m_desktopEntries;
    std::unique_ptr<Er::Private::IconManager> m_iconManager;
    std::unique_ptr<Er::Private::ProcessList> m_processList;
    std::unique_ptr<Er::Private::ProcessDetails> m_processDetails;
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

} // extern "C" {}