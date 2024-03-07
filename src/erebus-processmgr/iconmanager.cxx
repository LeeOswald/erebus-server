#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/file.hxx>

#include "desktopentries.hxx"
#include "iconcache.hxx"
#include "iconmanager.hxx"



namespace Er
{

namespace Private
{

IconManager::IconManager(Er::Log::ILog* log, IconCache* iconCache, DesktopEntries* desktopEntries, size_t cacheSize)
    : m_log(log)
    , m_iconCache(iconCache)
    , m_desktopEntries(desktopEntries)
    , m_cache16(cacheSize)
    , m_cache32(cacheSize)
{
    
}

void IconManager::prefetch(IconSize size)
{
    auto iconList = m_desktopEntries->iconList();
    m_iconCache->prefetch(iconList, static_cast<unsigned>(size));
}

std::shared_ptr<IconManager::IconData> IconManager::lookup(const std::string& exe, IconSize size) const noexcept
{
    // first look in in-memory cache
    Er::LruCache<std::string, std::shared_ptr<IconData>>* cache = (size == IconSize::Large) ? &m_cache32 : &m_cache16;
    {
        std::shared_lock l(m_mutex);
        auto cached = cache->get(exe);
        if (cached)
        {
            LogDebug(m_log, LogComponent("IconManager"), "Found cached icon for [%s]", exe.c_str());
            return *cached;
        }
    }

    // lookup the desktop entry
    auto desktop = m_desktopEntries->lookup(exe);
    if (!desktop)
    {
        // no icon for this exe
        std::unique_lock l(m_mutex);
        auto dummy = std::make_shared<IconData>();
        cache->put(exe, dummy);
        LogDebug(m_log, LogComponent("IconManager"), "No registered icon for [%s]", exe.c_str());
        return dummy;
    }

    // maybe it is cached on disk
    auto cachePath = m_iconCache->lookup(desktop->icon, static_cast<unsigned>(size));
    if (!cachePath)
    {
        // no icon for this exe
        std::unique_lock l(m_mutex);
        auto dummy = std::make_shared<IconData>();
        cache->put(exe, dummy);
        LogDebug(m_log, LogComponent("IconManager"), "No cached icon for [%s]", exe.c_str());
        return dummy;
    }

    // load from the disk cache
    auto data = Er::protectedCall<Bytes>(
        m_log,
        LogComponent("IconManager"),
        [this, cachePath]()
        {
            return Er::Util::loadBinaryFile(*cachePath);
        }
    );

    if (data.bytes.empty())
    {
        // no icon for this exe
        std::unique_lock l(m_mutex);
        auto dummy = std::make_shared<IconData>();
        cache->put(exe, dummy);
        LogDebug(m_log, LogComponent("IconManager"), "No valid icon for [%s]", exe.c_str());
        return dummy;
    }

    // cache this icon in memory
    auto ico = std::make_shared<IconData>(std::move(data));
    std::unique_lock l(m_mutex);
    cache->put(exe, ico);
    LogDebug(m_log, LogComponent("IconManager"), "Found icon for [%s]", exe.c_str());

    return ico;
}

} // namespace Private {}

} // namespace Er {}