#include <erebus/exception.hxx>
#include <erebus-processmgr/processmgr.hxx>
#include <erebus-processmgr/processprops.hxx>

#include <atomic>

namespace Er
{

namespace
{

class ProcessMgrPlugin final
    : public Er::Server::IPlugin
    , public boost::noncopyable
{
public:
    ~ProcessMgrPlugin()
    {
        Er::ProcessProps::Private::unregisterAll();

        g_instances--;
    }

    explicit ProcessMgrPlugin(const Er::Server::PluginParams& params)
        : m_params(params)
    {
        long expected = 0;
        if (!g_instances.compare_exchange_strong(expected, 1, std::memory_order_acq_rel))
            throw Er::Exception(ER_HERE(), "Only one instance of erebus-processmgr plugin can be instantiated");

        Er::ProcessProps::Private::registerAll();
    }

private:
    static std::atomic<long> g_instances;

    Er::Server::PluginParams m_params;
};

std::atomic<long> ProcessMgrPlugin::g_instances = 0;


} // namespace {}

} // namespace Er {}


extern "C"
{

ER_PROCESSMGR_EXPORT std::shared_ptr<Er::Server::IPlugin> createPlugin(const Er::Server::PluginParams& params)
{
    return std::make_shared<Er::ProcessMgrPlugin>(params);
}

} // extern "C" {}