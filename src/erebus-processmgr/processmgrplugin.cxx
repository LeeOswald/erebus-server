#include <erebus/exception.hxx>
#include <erebus-processmgr/erebus-processmgr.hxx>

#include "processdetailsservice.hxx"
#include "processlistservice.hxx"

#include "erebus-version.h"

#include <atomic>

namespace Erp
{

namespace ProcessMgr
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

        Er::ProcessMgr::ProcessProps::Private::unregisterAll(m_params.log);
        Er::ProcessMgr::ProcessesGlobal::Private::unregisterAll(m_params.log);

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
            throwGenericError("Only one instance of erebus-processmgr plugin can be created");

        // create and register services
        m_processList.reset(new Erp::ProcessMgr::ProcessListService(m_params.log));
        m_processDetails.reset(new Erp::ProcessMgr::ProcessDetailsService(m_params.log));
        
        for (auto container: m_params.containers)
        {
            m_processList->registerService(container);
            m_processDetails->registerService(container);
        }

        Er::ProcessMgr::ProcessesGlobal::Private::registerAll(m_params.log);
        Er::ProcessMgr::ProcessProps::Private::registerAll(m_params.log);
    }

private:
    static std::atomic<long> g_instances;

    Er::Server::PluginParams m_params;
    std::unique_ptr<Erp::ProcessMgr::ProcessListService> m_processList;
    std::unique_ptr<Erp::ProcessMgr::ProcessDetailsService> m_processDetails;
};

std::atomic<long> ProcessMgrPlugin::g_instances = 0;


} // namespace ProcessMgr {}

} // namespace Erp {}


extern "C"
{

ER_PROCESSMGR_EXPORT Er::Server::IPlugin* createPlugin(const Er::Server::PluginParams& params)
{
    return new Erp::ProcessMgr::ProcessMgrPlugin(params);
}

} // extern "C" {}