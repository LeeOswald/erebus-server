#include <erebus/system/thread.hxx>
#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/file.hxx>


#include "iconcache.hxx"


namespace Er
{

namespace Desktop
{

namespace Private
{


IconCache::~IconCache()
{
}

IconCache::IconCache(Er::Log::ILog* log, std::shared_ptr<Er::Desktop::IIconCacheIpc> iconCacheIpc, size_t cacheSize)
    : m_log(log)
    , m_iconCacheIpc(iconCacheIpc)
    , m_cache16(cacheSize)
    , m_cache32(cacheSize)
    , m_iconWorker([this](std::stop_token stop) { iconWorker(stop); })
{
    
}

IconCache::IconState IconCache::requestIcon(const std::string& name, IconSize size) noexcept
{
    auto& list = (size == IconSize::Large) ? m_appIcons32 : m_appIcons16;
    try
    {
        // check if already requested
        {
            std::unique_lock l(m_appIconsLock);
            auto existing = list.find(name);
            if (existing != list.end())
            {
                if (existing->second->state == IconState::Requested)
                { 
                    if (IconInfo::Clock::now() - existing->second->timestamp < IconRequestExpired)
                    {
                        Er::Log::Debug(m_log) << "Icon [" << name << "] has already been requested";
                        return IconState::Requested; // already requested
                    }
                }
                
                // if we're here, we need to re-request the icon
                list.erase(existing);
            }
        }

        if (m_iconCacheIpc->requestIcon(Er::Desktop::IIconCacheIpc::IconRequest(name, uint16_t(size))))
        {
            Er::Log::Debug(m_log) << "Requested icon [" << name << "]";

            // mark icon as 'requested'
            {
                std::unique_lock l(m_appIconsLock);

                list.insert({ name, std::make_shared<IconInfo>(IconState::Requested) });
            }

            return IconState::Requested;
        }
    }
    catch (Er::Exception& e)
    {
        Er::Util::logException(m_log, Er::Log::Level::Error, e);
    }
    catch (std::exception& e)
    {
        Er::Util::logException(m_log, Er::Log::Level::Error, e);
    }

    Er::Log::Warning(m_log) << "requestIcon() failed";
    return IconState::Failure;
}

void IconCache::iconWorker(std::stop_token stop) noexcept
{
    Er::Log::Debug(m_log) << "Icon cache worker started";
    Er::System::CurrentThread::setName("IconWorker");

    while (!stop.stop_requested())
    {
        receiveIcon();
    }

    Er::Log::Debug(m_log) << "IconWorker exited";
}

void IconCache::receiveIcon() noexcept
{
    try
    {
        auto response = m_iconCacheIpc->pullIcon(Timeout);
        if (response)
        {
            auto& list = (response->request.size == uint16_t(IconSize::Large)) ? m_appIcons32 : m_appIcons16;
            
            std::unique_lock l(m_appIconsLock);
            auto existing = list.find(response->request.name);
            if (existing != list.end())
            {
                list.erase(existing);
            }

            if (response->result != Er::Desktop::IIconCacheIpc::IconResponse::Result::Ok)
            {
                Er::Log::Info(m_log) << "Icon [" << response->request.name << "] was not found";
                list.insert({ response->request.name, std::make_shared<IconInfo>(IconState::NotFound) });
            }
            else
            {
                Er::Log::Info(m_log) << "Found icon [" << response->request.name << "] -> [" << response->path << "]";
                list.insert({ response->request.name, std::make_shared<IconInfo>(response->path) });
            }
        }
    }
    catch (Er::Exception& e)
    {
        Er::Util::logException(m_log, Er::Log::Level::Error, e);
    }
    catch (std::exception& e)
    {
        Er::Util::logException(m_log, Er::Log::Level::Error, e);
    }
}

std::shared_ptr<IconCache::IconData> IconCache::lookupByName(const std::string& name, IconSize size)
{
    // retrieve icon path
    std::string path;
    {
        std::shared_lock l(m_appIconsLock);

        auto& appIcons = (size == IconSize::Large) ? m_appIcons32 : m_appIcons16;
        auto existing = appIcons.find(name);

        if (existing != appIcons.end())
            path = existing->second->path; 
    }

    if (path.empty())
    {
        // icon path yet unknown; request it now
        auto state = requestIcon(name, size);
        return std::make_shared<IconData>(state);
    }

    // check if icon bytes are already cached
    {
        std::unique_lock l(m_cacheLock);

        auto& cache = (size == IconSize::Large) ? m_cache32 : m_cache16;
        auto existing = cache.get(path);

        if (existing)
        {
            Er::Log::Debug(m_log) << "Found cached icon [" << path << "]";
            return *existing;
        }

        // load icon from cache file
        auto data = Er::protectedCall<Bytes>(
            m_log,
            ErLogComponent("IconCache"),
            [this, &path]()
            {
                return Er::Util::loadBinaryFile(path);
            }
        );

        if (data.empty())
        {
            // failed for whatever reason; don't try to load this icon file again
            auto dummy = std::make_shared<IconData>(IconState::Failure);
            cache.put(path, dummy);
            Er::Log::Error(m_log) << "Could not load icon file [" << path << "]";
            return dummy;
        }

        // add to LRU cache
        auto icon = std::make_shared<IconData>(std::move(data));
        cache.put(path, icon);
        Er::Log::Info(m_log) << "Cached icon [" << path << "]";
        return icon;
    }
}


} // namespace Private {}

} // namespace Desktop {}

} // namespace Er {}