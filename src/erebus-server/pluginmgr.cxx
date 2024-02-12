#include "pluginmgr.hxx"


#include <erebus/exception.hxx>
#include <erebus/util/format.hxx>


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

std::shared_ptr<Er::Server::IPlugin> PluginMgr::load(const std::string& path)
{
    auto info = std::make_shared<PluginInfo>(path, m_params.log);
    
    boost::system::error_code ec;
    info->dll.load(path, boost::dll::load_mode::default_mode, ec);
    if (ec)
    {
        throw Er::Exception(ER_HERE(), Er::Util::format("Failed to load plugin [%s]", path.c_str()), Er::ExceptionProps::DecodedError(ec.message()));
    }

    if (!info->dll.has("createPlugin"))
    {
        throw Er::Exception(ER_HERE(), Er::Util::format("No createPlugin symbol found in [%s]", path.c_str()));
    }

    auto entry = info->dll.get<std::shared_ptr<Er::Server::IPlugin>(const Er::Server::PluginParams&)>("createPlugin");
    assert(entry);
    info->ref = entry(m_params);
    if (!info->ref)
    {
        throw Er::Exception(ER_HERE(), Er::Util::format("createPlugin of [%s] returned NULL", path.c_str()));
    }

    m_plugins.push_back(info);

    m_params.log->write(Er::Log::Level::Info, "Loaded plugin [%s]", path.c_str());

    return info->ref;
}


} // namespace Private {}

} // namespace Er {}