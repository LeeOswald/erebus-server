#include <erebus/exception.hxx>
#include <erebus/util/file.hxx>
#include <erebus/util/format.hxx>

#include <filesystem>

#include <boost/iostreams/device/mapped_file.hpp>


namespace Er
{

namespace Util
{

EREBUS_EXPORT std::string loadTextFile(const std::string& path)
{
    auto bytes = loadBinaryFile(path);
    return std::string(std::move(bytes.bytes()));
}

EREBUS_EXPORT Bytes loadBinaryFile(const std::string& path)
{
    boost::iostreams::mapped_file_source file(path);
    if (!file.is_open())
        throw Er::Exception(ER_HERE(), Er::Util::format("Failed to open [%s]", path.c_str()));

    std::string buffer(file.data(), file.size());

    return Bytes(std::move(buffer));
}

EREBUS_EXPORT std::string resolveSymlink(const std::string& path, unsigned maxDepth) noexcept
{
    std::filesystem::path fspath(path);
    std::error_code ec;
    while (maxDepth && std::filesystem::is_symlink(fspath, ec) && !ec)
    {
        fspath = std::filesystem::read_symlink(fspath, ec);
        --maxDepth;
    }

    return fspath.native();
}


} // namespace Util {}

} // namespace Er {]