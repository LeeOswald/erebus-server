#include <erebus/exception.hxx>
#include <erebus-processmgr/erebus-processmgr.hxx>

#include "processdetailsservice.hxx"
#include "processlistservice.hxx"

#include "erebus-version.h"

#include <atomic>

namespace Erp::ProcessMgr
{

class ProcessMgrPlugin final
    : public Er::Server::IPlugin
    , public Er::NonCopyable
{
public:
    ~ProcessMgrPlugin()
    {
        m_processList->unregisterService(m_params.container);
        m_processDetails->unregisterService(m_params.container);

        m_processList.reset();
        m_processDetails.reset();

        Er::ProcessMgr::Private::unregisterAll(m_params.log);

        g_instances--;
    }

    void dispose() noexcept override
    {
        delete this;
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
            ErThrow("Only one instance of erebus-processmgr plugin can be created");

        // create and register services
        m_processList = Erp::ProcessMgr::ProcessListService::create(m_params.log);
        m_processDetails = Erp::ProcessMgr::ProcessDetailsService::create(m_params.log);
        
        m_processList->registerService(m_params.container);
        m_processDetails->registerService(m_params.container);

        Er::ProcessMgr::Private::registerAll(m_params.log);
    }

private:
    static std::atomic<long> g_instances;

    Er::Server::PluginParams m_params;
    Er::Server::IService::Ptr m_processList;
    Er::Server::IService::Ptr m_processDetails;
};

std::atomic<long> ProcessMgrPlugin::g_instances = 0;


} // namespace Erp::ProcessMgr {}


extern "C"
{

ER_PROCESSMGR_EXPORT Er::Server::IPlugin* createPlugin(const Er::Server::PluginParams& params)
{
    return new Erp::ProcessMgr::ProcessMgrPlugin(params);
}

} // extern "C" {}