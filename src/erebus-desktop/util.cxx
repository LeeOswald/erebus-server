#include <erebus/util/sha256.hxx>
#include <erebus-desktop/erebus-desktop.hxx>


#include <filesystem>



namespace Er
{

namespace Desktop
{


EREBUSDESKTOP_EXPORT std::string makeIconCachePath(const std::string& cacheDir, const std::string& name, unsigned size, std::string_view ext)
{
    std::filesystem::path path(cacheDir);
    auto sz = std::to_string(size);
    
    // make path like /tmp/iconcache/39534cecc15cd261e9eb3c8cd3544d4f839db599979a9348bf54afc896a25ce8_32x32.png
    Er::Util::Sha256 hash;
    hash.update(name);
    auto hashStr = hash.str(hash.digest());

    std::string fileName(std::move(hashStr));
    fileName.append("_");
    fileName.append(sz);
    fileName.append("x");
    fileName.append(sz);
    fileName.append(ext);

    path.append(fileName);

    return path.string();
}

} // namespace Desktop {}

} // namespace Er {}
