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
                    throw Exception(
                        std::source_location::current(),
                        Error(Result::BadPlugin, GenericError),
                        Exception::Message("createPlugin() returned NULL"),
                        ExceptionProperties::ObjectName(info->path)
                    );
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
        throw Exception(std::source_location::current(), Error(ec), ExceptionProperties::ObjectName(path));
    }

    if (!info->dll.has("createPlugin"))
    {
        throw Exception(
            std::source_location::current(),
            Error(Result::BadPlugin, GenericError),
            Exception::Message("No createPlugin symbol found"),
            ExceptionProperties::ObjectName(info->path)
        );
    }

    auto entry = info->dll.get<Er::Server::CreatePluginFn>("createPlugin");
    ErAssert(entry);

    auto handle = entry(this, m_log, args);
    if (!handle)
    {
        throw Exception(
            std::source_location::current(),
            Error(Result::BadPlugin, GenericError),
            Exception::Message("reatePlugin returned NULL"),
            ExceptionProperties::ObjectName(info->path)
        );
    }

    info->entry = entry;

    {
        std::lock_guard l(m_mutex);
        m_plugins.push_back(std::move(info));
    }

    return handle;
}

} // namespace Er::Server {}

