#include "pluginmgr.hxx"


#include <erebus/exception.hxx>
#include <erebus/util/format.hxx>
#include <erebus/util/stringutil.hxx>


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
        throw Er::Exception(ER_HERE(), Er::Util::format("Failed to load plugin [%s]", params.binary.c_str()), Er::ExceptionProps::DecodedError(ec.message()));
    }

    if (!info->dll.has("createPlugin"))
    {
        throw Er::Exception(ER_HERE(), Er::Util::format("No createPlugin symbol found in [%s]", params.binary.c_str()));
    }

    if (!info->dll.has("disposePlugin"))
    {
        throw Er::Exception(ER_HERE(), Er::Util::format("No disposePlugin symbol found in [%s]", params.binary.c_str()));
    }

    info->disposeFn = info->dll.get<Er::Server::disposePlugin>("disposePlugin");
    assert(info->disposeFn);

    auto entry = info->dll.get<Er::Server::createPlugin>("createPlugin");
    assert(entry);


    info->ref = entry(params);
    if (!info->ref)
    {
        throw Er::Exception(ER_HERE(), Er::Util::format("createPlugin of [%s] returned NULL", params.binary.c_str()));
    }

    {
        std::lock_guard l(m_mutex);
        m_plugins.push_back(info);
    }

    m_params.log->write(Er::Log::Level::Info, LogNowhere(), "Loaded plugin [%s]", params.binary.c_str());

    return info->ref;
}

void PluginMgr::unloadAll()
{
    std::lock_guard l(m_mutex);
    m_plugins.clear();
}


} // namespace Private {}

} // namespace Er {}