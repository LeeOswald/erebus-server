#include <erebus/exception.hxx>
#include <erebus/util/file.hxx>
#include <erebus/util/format.hxx>

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

} // namespace Util {}

} // namespace Er {]