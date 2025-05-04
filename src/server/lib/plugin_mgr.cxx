#include <erebus/rtl/exception.hxx>
#include <erebus/server/plugin_mgr.hxx>


namespace Er::Server
{

PluginPtr PluginMgr::loadPlugin(const std::string& path, const PropertyMap& args)
{
    // check if already loaded
    {
        std::unique_lock l(m_mutex);
        for (auto& info : m_plugins)
        {
            if (info->path == path)
            {
                auto handle = info->entry(this, m_log, args);
                if (!handle)
                {
                    ErThrow(Er::format("createPlugin of [{}] returned NULL", info->path));
                }

                return handle;
            }
        }
    }

    // try loading
    auto info = std::make_unique<PluginModule>(path);

    boost::system::error_code ec;
    info->dll.load(path, boost::dll::load_mode::default_mode, ec);
    if (ec)
    {
#if ER_LINUX
        auto err = ::dlerror();
        if (err)
            Er::Log::writeln(m_log.get(), Log::Level::Error, err);
#endif
        ErThrow(Er::format("Failed to load [{}]", path), Property{ ExceptionProps::DecodedError, ec.message() });
    }

    if (!info->dll.has("createPlugin"))
    {
        ErThrow(Er::format("No createPlugin symbol found in [{}]", path));
    }

    info->entry = info->dll.get<Er::Server::CreatePluginFn>("createPlugin");
    ErAssert(info->entry);

    auto handle = info->entry(this, m_log, args);
    if (!handle)
    {
        ErThrow(Er::format("createPlugin of [{}] returned NULL", info->path));
    }

    {
        std::lock_guard l(m_mutex);
        m_plugins.push_back(std::move(info));
    }

    return handle;
}

} // namespace Er::Server {}

