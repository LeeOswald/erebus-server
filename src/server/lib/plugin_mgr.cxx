#include <erebus/rtl/util/exception_util.hxx>
#include <erebus/server/plugin_mgr.hxx>


namespace Er::Private
{

IPlugin* PluginMgr::load(const std::string& path, const PropertyBag& args)
{
    auto info = std::make_unique<PluginInfo>(path);

    boost::system::error_code ec;
    info->dll.load(path, boost::dll::load_mode::default_mode, ec);
    if (ec)
    {
#if ER_LINUX
        auto err = ::dlerror();
        if (err)
            Er::Log::writeln(m_log.get(), Log::Level::Error, err);
#endif
        ErThrow(Er::format("Failed to load plugin [{}]", path), Property{ ExceptionProps::DecodedError, ec.message() });
    }

    if (!info->dll.has("createPlugin"))
    {
        ErThrow(Er::format("No createPlugin symbol found in [{}]", path));
    }

    auto entry = info->dll.get<Er::CreatePluginFn>("createPlugin");
    ErAssert(entry);

    info->ptr = entry(m_owner, m_log, args);
    if (!info->ptr)
    {
        ErThrow(Er::format("createPlugin of [{}] returned NULL", path));
    }

    Util::ExceptionLogger xcptHandler(m_log.get());
    try
    {
        auto props = info->ptr->info();

        ErLogIndent2(m_log.get(), Log::Level::Info, "Loaded plugin {}", path);
        for (auto& prop : props)
        {
            ErLogInfo2(m_log.get(), "{}: {}", prop.name(), prop.str());
        }

    }
    catch (...)
    {
        dispatchException(std::current_exception(), xcptHandler);
        ErLogError2(m_log.get(), "Failed to load plugin [{}]", path);

        return {};
    }

    {
        std::lock_guard l(m_mutex);
        m_plugins.push_back(std::move(info));
    }

    return info->ptr;
}

} // namespace Er::Private {}