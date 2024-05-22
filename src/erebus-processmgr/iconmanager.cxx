#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/file.hxx>
#include <erebus-desktop/desktopentries.hxx>

#include "iconcache.hxx"
#include "iconmanager.hxx"

#include <regex>


namespace Er
{

namespace Private
{

namespace
{

struct KnownApp
{
    std::regex command;
    std::string icon;
};

const KnownApp g_KnownApps[] =
{
    { std::regex("^((ba|z|tc|c|k)?sh)$"), "utilities-terminal" }
};

std::optional<std::string> findKnownAppIcon(const std::string& app)
{
    for (auto& k: g_KnownApps)
    {
        if (regex_match(app, k.command))
            return k.icon;
    }

    return std::nullopt;
}

}

IconManager::IconManager(Er::Log::ILog* log, IconCache* iconCache, Er::Desktop::DesktopEntries* desktopEntries, size_t cacheSize)
    : m_log(log)
    , m_iconCache(iconCache)
    , m_desktopEntries(desktopEntries)
    , DefaultExeIcon("application-x-executable")
    , m_cache16(cacheSize)
    , m_cache32(cacheSize)
{
    
}

void IconManager::prefetch(IconSize size)
{
    auto iconList = m_desktopEntries->iconList();
    m_iconCache->prefetch(iconList, static_cast<unsigned>(size));
}

std::shared_ptr<IconManager::IconData> IconManager::defaultIcon(const std::string& comm, const std::string& exe, IconSize size) const noexcept
{
    std::string name;

    if (comm.empty())
    {
        name = DefaultExeIcon;
    }
    else
    {
        auto knownIcon = findKnownAppIcon(comm);
        name = knownIcon ? *knownIcon : DefaultExeIcon;
    }

    // look for cached default icon
    auto cache = (size == IconSize::Large) ? &m_default32 : &m_default16;
    {
        std::shared_lock l(m_mutex);
        auto cached = cache->find(name);
        if (cached != cache->end())
        {
            ErLogDebug(m_log, ErLogComponent("IconManager"), "Found cached default icon [%s] for [%s] [%s]", name.c_str(), comm.c_str(), exe.c_str());
            return cached->second;
        }
    }

    // ask the system for the (default) icon
    auto cachePath = m_iconCache->lookup(name, static_cast<unsigned>(size));
    if (!cachePath)
    {
        std::unique_lock l(m_mutex);
        auto dummy = std::make_shared<IconData>();
        cache->insert({ name, dummy });
        ErLogWarning(m_log, ErLogComponent("IconManager"), "No default icon [%s] for [%s] [%s]", name.c_str(), comm.c_str(), exe.c_str());
        return dummy;
    }

    // load from the disk cache
    auto data = Er::protectedCall<Bytes>(
        m_log,
        ErLogComponent("IconManager"),
        [this, cachePath]()
        {
            return Er::Util::loadBinaryFile(*cachePath);
        }
    );

    if (data.empty())
    {
        std::unique_lock l(m_mutex);
        auto dummy = std::make_shared<IconData>();
        cache->insert({ name, dummy });
        ErLogWarning(m_log, ErLogComponent("IconManager"), "Default icon [%s] could not be loaded", name.c_str());
        return dummy;
    }

    // cache it
    std::unique_lock l(m_mutex);
    auto result = std::make_shared<IconData>(std::move(data.bytes()));
    cache->insert({ name, result });
    ErLogDebug(m_log, ErLogComponent("IconManager"), "Using default icon [%s] for [%s] [%s]", name.c_str(), comm.c_str(), exe.c_str());

    return result;
}

std::shared_ptr<IconManager::IconData> IconManager::lookup(const std::string& comm, const std::string& exe, IconSize size) const noexcept
{
    // first look in in-memory cache
    auto cache = (size == IconSize::Large) ? &m_cache32 : &m_cache16;
    {
        std::shared_lock l(m_mutex);
        auto cached = cache->get(exe);
        if (cached)
        {
            ErLogDebug(m_log, ErLogComponent("IconManager"), "Found cached icon for [%s] [%s]", comm.c_str(), exe.c_str());
            return *cached;
        }
    }

    // lookup the desktop entry
    auto desktop = m_desktopEntries->lookup(exe);
    if (!desktop)
    {
        // no icon for this exe
        return defaultIcon(comm, exe, size);
    }

    // maybe it is cached on disk
    auto cachePath = m_iconCache->lookup(desktop->icon, static_cast<unsigned>(size));
    if (!cachePath)
    {
        // no icon for this exe
        return defaultIcon(comm, exe, size);
    }

    // load from the disk cache
    auto data = Er::protectedCall<Bytes>(
        m_log,
        ErLogComponent("IconManager"),
        [this, cachePath]()
        {
            return Er::Util::loadBinaryFile(*cachePath);
        }
    );

    if (data.empty())
    {
        // no icon for this exe
        return defaultIcon(comm, exe, size);
    }

    // cache this icon in memory
    auto ico = std::make_shared<IconData>(std::move(data));
    std::unique_lock l(m_mutex);
    cache->put(exe, ico);
    ErLogDebug(m_log, ErLogComponent("IconManager"), "Found icon for [%s] [%s]", comm.c_str(), exe.c_str());

    return ico;
}

} // namespace Private {}

} // namespace Er {}