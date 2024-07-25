#include <erebus-desktop/protocol.hxx>
#include <erebus/system/thread.hxx>
#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/file.hxx>
#include <erebus/util/format.hxx>

#include "iconcache.hxx"

#include <filesystem>
#include <sys/stat.h>

namespace Er
{

namespace Desktop
{

namespace Private
{


IconCache::~IconCache()
{
}

IconCache::IconCache(Er::Log::ILog* log, std::shared_ptr<Er::Desktop::IIconCacheIpc> iconCacheIpc, const std::string& cacheDir, size_t cacheSize)
    : m_log(log)
    , m_iconCacheIpc(iconCacheIpc)
    , m_cacheDir(cacheDir)
    , m_cache16(cacheSize)
    , m_cache32(cacheSize)
    , m_iconWorker([this](std::stop_token stop) { iconWorker(stop); })
{
    std::filesystem::path path(m_cacheDir);
    std::error_code ec;
    if (std::filesystem::exists(path))
    {
        if (!std::filesystem::is_directory(path, ec) || ec)
            throwGenericError(Er::Util::format("%s is not a directry", m_cacheDir.c_str()));

        if (::access(m_cacheDir.c_str(), R_OK | W_OK) == -1)
            throwGenericError(Er::Util::format("%s is inaccessible", m_cacheDir.c_str()));
    }
    else
    {
        if (!std::filesystem::create_directory(path, ec) || ec)
            throwGenericError(Er::Util::format("Failed to create %s: %s", m_cacheDir.c_str(), ec.message().c_str()));
    }
}

std::shared_ptr<IconCache::IconInfo> IconCache::searchCacheDir(const std::string& name, IconSize size) noexcept
{
    auto iconPath = Er::Desktop::makeIconCachePath(m_cacheDir, name, static_cast<unsigned>(size), ".png");

    struct stat st = {};
    if (::stat(iconPath.c_str(), &st) == -1)
        return std::shared_ptr<IconCache::IconInfo>();

    auto& list = (size == IconSize::Large) ? m_appIcons32 : m_appIcons16;
        
    std::unique_lock l(m_appIconsLock);
    auto existing = list.find(name);
    if (existing != list.end())
    {
        return existing->second;
    }

    Er::Log::Info(m_log) << "Found icon [" << name << "] -> [" << iconPath << "]";

    auto info = std::make_shared<IconInfo>(std::move(iconPath));
    list.insert({ name, info });

    return info;
}

std::shared_ptr<IconCache::IconInfo> IconCache::requestIcon(const std::string& name, IconSize size) noexcept
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
                if (existing->second->state == IconState::Pending)
                { 
                    if (IconInfo::Clock::now() - existing->second->timestamp < IconRequestExpired)
                    {
                        Er::Log::Debug(m_log) << "Icon [" << name << "] has already been requested";
                        return existing->second; // already requested
                    }
                }
                
                // if we're here, we need to re-request the icon
                list.erase(existing);
            }
        }

        if (m_iconCacheIpc->requestIcon(Er::Desktop::IIconCacheIpc::IconRequest(name, uint16_t(size)), Timeout))
        {
            Er::Log::Debug(m_log) << "Requested icon [" << name << "]";

            // mark icon as 'requested'
            {
                std::unique_lock l(m_appIconsLock);

                auto pending = std::make_shared<IconInfo>(IconState::Pending);
                auto it = list.insert({ name, pending });

                return it.first->second;
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

    Er::Log::Warning(m_log) << "requestIcon() failed";
    return std::make_shared<IconInfo>(IconState::NotPresent);
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
                list.insert({ response->request.name, std::make_shared<IconInfo>(IconState::NotPresent) });
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
    std::shared_ptr<IconInfo> iconInfo;
    {
        std::shared_lock l(m_appIconsLock);

        auto& appIcons = (size == IconSize::Large) ? m_appIcons32 : m_appIcons16;
        auto existing = appIcons.find(name);

        if (existing != appIcons.end())
            iconInfo = existing->second; 
    }

    if (!iconInfo && !m_cacheDir.empty())
        iconInfo = searchCacheDir(name, size); // maybe already in the file cache

    if (!iconInfo && m_iconCacheIpc)
        iconInfo = requestIcon(name, size); // icon path yet unknown; request it now
        
    if (!iconInfo)
        return std::make_shared<IconData>(IconState::NotPresent);
    
    if (iconInfo->state != IconState::Found)
        return std::make_shared<IconData>(iconInfo->state);

    // check if icon bytes are already cached
    {
        std::unique_lock l(m_cacheLock);

        auto& cache = (size == IconSize::Large) ? m_cache32 : m_cache16;
        auto existing = cache.get(iconInfo->path);

        if (existing)
        {
            Er::Log::Debug(m_log) << "Found cached icon [" << iconInfo->path << "]";
            return *existing;
        }

        // load icon from cache file
        auto data = Er::protectedCall<Bytes>(
            m_log,
            [this, iconInfo]()
            {
                return Er::Util::loadBinaryFile(iconInfo->path);
            }
        );

        if (data.empty())
        {
            // failed for whatever reason; don't try to load this icon file again
            auto dummy = std::make_shared<IconData>(IconState::NotPresent);
            cache.put(iconInfo->path, dummy);
            Er::Log::Error(m_log) << "Could not load icon file [" << iconInfo->path << "]";
            return dummy;
        }

        // add to LRU cache
        auto icon = std::make_shared<IconData>(std::move(data));
        cache.put(iconInfo->path, icon);
        Er::Log::Info(m_log) << "Cached icon [" << iconInfo->path << "]";
        return icon;
    }
}


} // namespace Private {}

} // namespace Desktop {}

} // namespace Er {}