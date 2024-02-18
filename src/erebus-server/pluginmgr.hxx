#pragma once

#include <erebus-srv/plugin.hxx>

#include <vector>

#include <boost/dll.hpp>


namespace Er
{

namespace Private
{


class PluginMgr final
    : public Er::NonCopyable
{
public:
    ~PluginMgr();
    explicit PluginMgr(const Er::Server::PluginParams& params);

    Er::Server::IPlugin* load(const std::string& path);

private:
    struct PluginInfo
        : public Er::NonCopyable
    {
        std::string path;
        Er::Log::ILog* log = nullptr;
        boost::dll::shared_library dll;
        Er::Server::IPlugin* ref = nullptr;
        Er::Server::disposePlugin* disposeFn = nullptr;
        
        ~PluginInfo()
        {
            if (disposeFn)
                disposeFn(ref);
                
            if (dll.is_loaded())
                log->write(Er::Log::Level::Info, LogNowhere(), "Unloading plugin [%s]", path.c_str());
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