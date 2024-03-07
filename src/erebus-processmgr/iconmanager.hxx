#pragma once

#include <erebus/log.hxx>
#include <erebus/lrucache.hxx>
#include <erebus-processmgr/processmgr.hxx>

#include <shared_mutex>

namespace Er
{

namespace Private
{

class IconCache;
class DesktopEntries;

enum class IconSize : unsigned
{
    Small = 16,
    Large = 32
};


class IconManager final
    : public Er::NonCopyable
{
public:
    explicit IconManager(Er::Log::ILog* log, IconCache* iconCache, DesktopEntries* desktopEntries, size_t cacheSize);

    void prefetch(IconSize size);

private:
    struct IconData
    {

    };

    Er::Log::ILog* const m_log;
    IconCache* m_iconCache;
    DesktopEntries* m_desktopEntries;
    std::shared_mutex m_mutex;
    Er::LruCache<std::string, std::shared_ptr<IconData>> m_cache; // exec -> icon
};
    

} // namespace Private {}

} // namespace Er {}