#include <erebus/exception.hxx>
#include <erebus-desktop/erebus-desktop.hxx>
#include <erebus-srv/plugin.hxx>

#include "desktopfilecache.hxx"
#include "erebus-version.h"
#include "iconcache.hxx"
#include "iconresolver.hxx"
#include "service.hxx"

#include <atomic>

namespace Er
{

namespace Desktop
{

namespace
{

class DesktopPlugin final
    : public Er::Server::IPlugin
    , public Er::NonCopyable
{
public:
    ~DesktopPlugin()
    {
        // unregister services
        for (auto container: m_params.containers)
        {
            m_service->unregisterService(container);
        }

        Props::Private::unregisterAll(m_params.log);

        g_instances--;
    }

    void dispose() noexcept override
    {
        delete this;
    }

    Er::Server::IPlugin::Info info() const override
    {
        return Er::Server::IPlugin::Info(
            "Desktop",
            "Desktop application properties",
            Er::Server::IPlugin::Info::Version(ER_VERSION_MAJOR, ER_VERSION_MINOR, ER_VERSION_PATCH)
        );
    }

    explicit DesktopPlugin(const Er::Server::PluginParams& params)
        : m_params(params)
    {
        long expected = 0;
        if (!g_instances.compare_exchange_strong(expected, 1, std::memory_order_acq_rel))
            ErThrow("Only one instance of erebus-desktop plugin can be created");

        Props::Private::registerAll(params.log);

        auto args = parseArgs(params);

        if (!args.iconCacheMq.empty())
        {
            std::string mqIn(args.iconCacheMq);
            mqIn.append("_resp");
            std::string mqOut(args.iconCacheMq);
            mqOut.append("_req");

            m_iconCacheIpc = Er::Desktop::createIconCacheIpc(mqIn.c_str(), mqOut.c_str(), MaxIconCacheQueue);
        }
        else
        {
            params.log->write(Er::Log::Level::Warning, "Starting without icon cache");
        }

        m_desktopFileCache = std::make_shared<Erp::Desktop::DesktopFileCache>(params.log);

        m_iconResolver = std::make_shared<Erp::Desktop::IconResolver>(params.log, m_desktopFileCache);

        if (m_iconCacheIpc || !args.iconCacheDir.empty())
        {
            m_iconCache = std::make_shared<Erp::Desktop::IconCache>(params.log, m_iconCacheIpc, args.iconCacheDir, args.iconCacheSize);
        }

        m_service.reset(new Erp::Desktop::Service(m_params.log, m_iconResolver, m_iconCache));
        for (auto container: m_params.containers)
        {
            m_service->registerService(container);
        }
    }

private:
    struct PluginArgs
    {
        std::string iconCacheDir;
        std::string iconCacheMq;
        size_t iconCacheSize;
    };

    PluginArgs parseArgs(const Er::Server::PluginParams& params)
    {
        PluginArgs a;

        for (auto arg = params.args.begin(); arg != params.args.end(); ++arg)
        {
            if (arg->name == "iconcachemq")
            {
                a.iconCacheMq = arg->value;
            }
            else if (arg->name == "iconcachesize")
            {
                a.iconCacheSize = std::strtoul(arg->value.c_str(), nullptr, 10);
            }
            else if (arg->name == "iconcachedir")
            {
                a.iconCacheDir = arg->value;
            }
            else
            {
                params.log->writef(Er::Log::Level::Error, "Unrecognized plugin arg [%s]", arg->name.c_str());
            }
        }

        a.iconCacheSize = std::clamp(a.iconCacheSize, size_t(1024), size_t(65536));
        return a;
    }

    static constexpr size_t MaxIconCacheQueue = 256;

    static std::atomic<long> g_instances;

    Er::Server::PluginParams m_params;
    std::shared_ptr<IIconCacheIpc> m_iconCacheIpc;
    std::shared_ptr<Erp::Desktop::DesktopFileCache> m_desktopFileCache;
    std::shared_ptr<Erp::Desktop::IconResolver> m_iconResolver;
    std::shared_ptr<Erp::Desktop::IconCache> m_iconCache;
    std::unique_ptr<Erp::Desktop::Service> m_service;
};

std::atomic<long> DesktopPlugin::g_instances = 0;


} // namespace {}

} // namespace Desktop {}

} // namespace Er {}


extern "C"
{

EREBUSDESKTOP_EXPORT Er::Server::IPlugin* createPlugin(const Er::Server::PluginParams& params)
{
    return new Er::Desktop::DesktopPlugin(params);
}

} // extern "C" {}