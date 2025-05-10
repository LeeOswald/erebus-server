#include <erebus/proctree/server/proctree.hxx>
#include <erebus/rtl/util/unknown_base.hxx>
#include <erebus/server/iplugin_host.hxx>

#include "erebus-version.h"
#include "proctree_service.hxx"


namespace Er::ProcessTree::Private
{


class ProctreePlugin final
    : public Util::ReferenceCountedBase<Util::ObjectBase<Server::IPlugin>>
{
    using Base = Util::ReferenceCountedBase<Util::ObjectBase<Server::IPlugin>>;

public:
    ~ProctreePlugin()
    {
        ErLogDebug2(m_log.get(), "{}.ProctreePlugin::~ProctreePlugin()", Er::Format::ptr(this));
    }

    explicit ProctreePlugin(Er::IUnknown* host, Er::Log::LoggerPtr log, const Er::PropertyMap& args)
        : m_host(host->queryInterface<Server::IPluginHost>())
        , m_log(Log::makeSyncLogger("proctree"))
        , m_args(args)
    {
        m_log->addSink("main", log.cast<Log::ISink>());

        ErLogDebug2(m_log.get(), "{}.ProctreePlugin::ProctreePlugin()", Er::Format::ptr(this));

        startServices();
    }

    Er::PropertyBag info() const override
    {
        Er::PropertyBag bag;
        Er::addProperty(bag, { Er::Server::PluginProps::VersionString, Er::format("{}.{}.{}", ER_VERSION_MAJOR, ER_VERSION_MINOR, ER_VERSION_PATCH) });
        Er::addProperty(bag, { Er::Server::PluginProps::Name, "procexp" });
        Er::addProperty(bag, { Er::Server::PluginProps::Brief, "Process Explorer" });
        return bag;
    }

private:
    void startServices()
    {
        auto grpcServer = m_host->server();

        auto proctreeSvc = createProcessListService(m_log.get());
        grpcServer->addService(proctreeSvc);
    }

    Server::PluginHostPtr m_host;
    Er::Log::LoggerPtr m_log;
    Er::PropertyMap const m_args;
};


} // Er::ProcessTree::Private {}


extern "C"
{

ER_PROCTREE_EXPORT Er::Server::PluginPtr createPlugin(Er::IUnknown* host, Er::Log::LoggerPtr log, const Er::PropertyMap& args)
{
    return Er::Server::PluginPtr { new Er::ProcessTree::Private::ProctreePlugin(host, log, args) };
}

} // extern "C" {}