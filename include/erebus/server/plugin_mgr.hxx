#pragma once

#include <erebus/rtl/log.hxx>
#include <erebus/rtl/property_bag.hxx>
#include <erebus/server/iplugin.hxx>
#include <erebus/server/server_lib.hxx>

#include <boost/dll.hpp>
#include <boost/noncopyable.hpp>

#include <mutex>
#include <vector>


namespace Er::Private
{


class  ER_SERVER_EXPORT PluginMgr final
    : public boost::noncopyable
{
public:
    ~PluginMgr()
    {
        m_plugins.clear();
    }
    
    explicit PluginMgr(IUnknown::Ptr owner, Log::ILogger::Ptr log)
        : m_owner(owner)
        , m_log(log)
    {
    }

    IPlugin::Ptr load(const std::string& path, const PropertyBag& args);

private:
    struct PluginInfo
        : public boost::noncopyable
    {
        std::string path;
        Log::ILogger* log;
        boost::dll::shared_library dll;
        IPlugin::Ptr ref;

        ~PluginInfo()
        {
            if (dll.is_loaded())
                Er::Log::info(log, "Unloading plugin [{}]", path);
        }

        explicit PluginInfo(const std::string& path, Log::ILogger* log)
            : path(path)
            , log(log)
        {
        }
    };

    IUnknown::Ptr m_owner;
    Log::ILogger::Ptr const m_log;
    std::mutex m_mutex;
    std::vector<std::unique_ptr<PluginInfo>> m_plugins;
};

} // namespace Er::Private {}
