#pragma once

#include <erebus/rtl/log.hxx>
#include <erebus/rtl/property_bag.hxx>
#include <erebus/server/iplugin.hxx>
#include <erebus/server/server_lib.hxx>

#include <boost/dll.hpp>
#include <boost/noncopyable.hpp>

#include <mutex>
#include <vector>


namespace Er::Server
{


class  ER_SERVER_EXPORT PluginMgr final
    : public boost::noncopyable
{
public:
    ~PluginMgr()
    {
        m_plugins.clear();
    }
    
    explicit PluginMgr(IUnknown* owner, Log::LoggerPtr log)
        : m_owner(owner)
        , m_log(log)
    {
    }

    IPlugin* load(const std::string& path, const PropertyBag& args);

private:
    struct PluginInfo
        : public boost::noncopyable
    {
        std::string path;
        boost::dll::shared_library dll;
        IPlugin* ptr = nullptr;

        ~PluginInfo() = default;

        explicit PluginInfo(const std::string& path) noexcept
            : path(path)
        {
        }
    };

    IUnknown* m_owner;
    Log::LoggerPtr const m_log;
    std::mutex m_mutex;
    std::vector<std::unique_ptr<PluginInfo>> m_plugins;
};

} // namespace Er::Server {}
