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
    struct IconData
    {
        bool valid; // no icon exists for this exe
        std::string data;

        IconData() noexcept
            : valid(false)
        {}

        explicit IconData(std::string&& data) noexcept
            : valid(true)
            , data(std::move(data))
        {}
    };

    explicit IconManager(Er::Log::ILog* log, IconCache* iconCache, DesktopEntries* desktopEntries, size_t cacheSize);

    void prefetch(IconSize size);
    std::shared_ptr<IconData> lookup(const std::string& exe, IconSize size) const noexcept;

private:
    Er::Log::ILog* const m_log;
    IconCache* m_iconCache;
    DesktopEntries* m_desktopEntries;
    mutable std::shared_mutex m_mutex;
    mutable Er::LruCache<std::string, std::shared_ptr<IconData>> m_cache16; // exec -> icon
    mutable Er::LruCache<std::string, std::shared_ptr<IconData>> m_cache32;
};
    

} // namespace Private {}

} // namespace Er {}