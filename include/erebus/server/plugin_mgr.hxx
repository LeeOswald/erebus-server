#pragma once


#include <erebus/rtl/log.hxx>
#include <erebus/rtl/property_bag.hxx>
#include <erebus/rtl/util/unknown_base.hxx>
#include <erebus/server/iplugin_host.hxx>
#include <erebus/server/server_lib.hxx>

#include <boost/dll.hpp>
#include <boost/noncopyable.hpp>

#include <mutex>
#include <vector>


namespace Er::Server
{


class ER_SERVER_EXPORT PluginMgr final
    : public Util::ReferenceCountedBase<Util::ObjectBase<IPluginHost>>
{
    using Base = Util::ReferenceCountedBase<Util::ObjectBase<IPluginHost>>;

public:
    ~PluginMgr()
    {
        m_plugins.clear();
    }
    
    explicit PluginMgr(Log::LoggerPtr log, Ipc::Grpc::IServer* server)
        : m_log(log)
        , m_server(server)
    {
    }

    PluginPtr loadPlugin(const std::string& path, const PropertyMap& args) override;

    Ipc::Grpc::IServer* server() noexcept override
    {
        return m_server;
    }

private:
    struct PluginModule
        : public boost::noncopyable
    {
        std::string path;
        boost::dll::shared_library dll;
        Er::Server::CreatePluginFn* entry = nullptr;

        ~PluginModule() = default;

        explicit PluginModule(const std::string& path) noexcept
            : path(path)
        {
        }
    };

    Log::LoggerPtr const m_log;
    Ipc::Grpc::IServer* const m_server;
    std::mutex m_mutex;
    std::vector<std::unique_ptr<PluginModule>> m_plugins;
};

} // namespace Er::Server {}
