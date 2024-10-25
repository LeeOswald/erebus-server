#include "pluginmgr.hxx"

#include <erebus/exception.hxx>


namespace Er
{

namespace Private
{

PluginMgr::~PluginMgr()
{
}

PluginMgr::PluginMgr(const Er::Server::PluginParams& params)
    : m_params(params)
{
}

Er::Server::IPlugin* PluginMgr::load(const std::string& path, const std::vector<Er::Server::PluginParams::Arg>& args)
{
    Er::Server::PluginParams params;
    params.containers = m_params.containers;
    params.log = m_params.log;
    params.binary = path;
    params.args = args;

    auto info = std::make_shared<PluginInfo>(params.binary, m_params.log);
    
    boost::system::error_code ec;
    info->dll.load(params.binary, boost::dll::load_mode::default_mode, ec);
    if (ec)
    {
#if ER_LINUX
        auto err = ::dlerror();
        if (err)
            ErLogError(m_params.log, "%s", err); 
#endif
        ErThrow(Er::format("Failed to load plugin [{}]", params.binary), Er::ExceptionProps::DecodedError(ec.message()));
    }

    if (!info->dll.has("createPlugin"))
    {
        ErThrow(Er::format("No createPlugin symbol found in [{}]", params.binary));
    }

    auto entry = info->dll.get<Er::Server::createPlugin>("createPlugin");
    ErAssert(entry);

    info->ref.reset(entry(params));
    if (!info->ref)
    {
        ErThrow(Er::format("createPlugin of [{}] returned NULL", params.binary));
    }

    {
        std::lock_guard l(m_mutex);
        m_plugins.push_back(info);
    }

    auto pi = info->ref->info();

    Er::Log::info(
        m_params.log,
        "Loaded plugin {} ver {}.{}.{} [{}] from [{}]", 
        pi.name, 
        pi.version.major,
        pi.version.minor,
        pi.version.patch,
        pi.description,
        params.binary
    );

    return info->ref.get();
}

void PluginMgr::unloadAll()
{
    std::lock_guard l(m_mutex);
    m_plugins.clear();
}


} // namespace Private {}

} // namespace Er {}