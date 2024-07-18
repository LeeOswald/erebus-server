#pragma once

#include <erebus-srv/plugin.hxx>

#include <mutex>
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

    Er::Server::IPlugin* load(const std::string& path, const std::vector<Er::Server::PluginParams::Arg>& args);
    void unloadAll();

private:
    struct PluginInfo
        : public Er::NonCopyable
    {
        std::string path;
        Er::Log::ILog* log = nullptr;
        boost::dll::shared_library dll;
        std::unique_ptr<Er::Server::IPlugin> ref;
       
        ~PluginInfo()
        {
            if (dll.is_loaded())
                log->writef(Er::Log::Level::Info, "Unloading plugin [%s]", path.c_str());
        }

        explicit PluginInfo(const std::string& path, Er::Log::ILog* log)
            : path(path)
            , log(log)
        {}
    };

    const Er::Server::PluginParams m_params;
    std::mutex m_mutex;
    std::vector<std::shared_ptr<PluginInfo>> m_plugins;
};


} // namespace Private {}

} // namespace Er {}