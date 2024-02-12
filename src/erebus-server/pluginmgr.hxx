#pragma once

#include <erebus-srv/plugin.hxx>

#include <vector>

#include <boost/dll.hpp>


namespace Er
{

namespace Private
{


class PluginMgr final
    : public boost::noncopyable
{
public:
    ~PluginMgr();
    explicit PluginMgr(const Er::Server::PluginParams& params);

    std::shared_ptr<Er::Server::IPlugin> load(const std::string& path);

private:
    struct PluginInfo
    {
        std::string path;
        Er::Log::ILog* log = nullptr;
        // note the member order: 'ref' must be released before 'dll' gets uloaded
        boost::dll::shared_library dll;
        std::shared_ptr<Er::Server::IPlugin> ref;
        
        ~PluginInfo()
        {
            if (dll.is_loaded())
                log->write(Er::Log::Level::Info, "Unloading plugin [%s]", path.c_str());
        }

        explicit PluginInfo(const std::string& path, Er::Log::ILog* log)
            : path(path)
            , log(log)
        {}
    };

    Er::Server::PluginParams m_params;
    std::vector<std::shared_ptr<PluginInfo>> m_plugins;
};


} // namespace Private {}

} // namespace Er {}