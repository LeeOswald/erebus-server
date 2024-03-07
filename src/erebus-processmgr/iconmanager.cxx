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
    , m_cache(cacheSize)
{
    
}

void IconManager::prefetch(IconSize size)
{
    auto iconList = m_desktopEntries->iconList();
    m_iconCache->prefetch(iconList, static_cast<unsigned>(size));
}

} // namespace Private {}

} // namespace Er {}