#include <erebus/util/sha256.hxx>
#include <erebus-processmgr/iconcache.hxx>

#include <filesystem>

#include <boost/process.hpp>

namespace Er
{

IconCache::IconCache(Er::Log::ILog* log, const std::string& iconCache, const std::string& iconCacheDir)
    : m_log(log)
    , m_iconCache(iconCache)
    , m_iconCacheDir(iconCacheDir)
{
}

std::optional<std::string> IconCache::lookup(const std::string& name, unsigned size)
{
    auto iconPath = makeCachePath(name, size);
    // maybe it's already in cache
    std::filesystem::path path(iconPath);
    if (std::filesystem::exists(path))
        return std::make_optional(std::move(iconPath));

    // call erebus-iconcache executable
    if (!callCacheAgent(name, size))
        return std::nullopt;

    if (std::filesystem::exists(path))
        return std::make_optional(std::move(iconPath));

    return std::nullopt;
}

std::string IconCache::makeCachePath(const std::string& name, unsigned size) const
{
    std::filesystem::path path(m_iconCacheDir);
    auto sz = std::to_string(size);
    if (std::filesystem::path(name).is_absolute())
    {
        // make path like /tmp/iconcache/39534cecc15cd261e9eb3c8cd3544d4f839db599979a9348bf54afc896a25ce8_32x32.png
        Er::Util::Sha256 hash;
        hash.update(name);
        auto hashStr = hash.str(hash.digest());

        std::string fileName(std::move(hashStr));
        fileName.append("_");
        fileName.append(sz);
        fileName.append("x");
        fileName.append(sz);
        fileName.append(".png");

        path.append(fileName);

        return path.string();
    }

    // make path like /tmp/iconcache/mtapp_32x32.png
    std::string fileName(name);
    fileName.append("_");
    fileName.append(sz);
    fileName.append("x");
    fileName.append(sz);
    fileName.append(".png");

    path.append(fileName);

    return path.string();
}

bool IconCache::callCacheAgent(const std::string& iconName, unsigned size) const
{
    auto result = boost::process::system(m_iconCache, std::string("--cache"), m_iconCacheDir, std::string("--size"), std::to_string(size), iconName);
    return (result > 0);
}

} // namespace Er {}