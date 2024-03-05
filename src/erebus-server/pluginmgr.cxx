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

Er::Server::IPlugin* PluginMgr::load(const std::string& pathAndArgs)
{
    auto params = makeParams(m_params, pathAndArgs);
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

Er::Server::PluginParams PluginMgr::makeParams(const Er::Server::PluginParams& source, const std::string& args)
{
    Er::Server::PluginParams params;
    params.containers = source.containers;
    params.log = source.log;

    auto splittedArgs = Er::Util::split(args, std::string_view(" "), Er::Util::SplitSkipEmptyParts);
    if (splittedArgs.empty())
        throw Er::Exception(ER_HERE(), Er::Util::format("Plugin args [%s] contain no binary path", args.c_str()));

    params.binary = std::move(splittedArgs[0]);
    for (size_t i = 1; i < splittedArgs.size(); ++i)
    {
        params.args.push_back(std::move(splittedArgs[i]));
    }

    return params;
}

} // namespace Private {}

} // namespace Er {}