#include <erebus/exception.hxx>
#include <erebus/util/file.hxx>
#include <erebus/util/format.hxx>

#include <fstream>
#include <sstream>

namespace Er
{

namespace Util
{

namespace
{

struct FileCloser
{
    void operator()(FILE* fd)
    {
        std::fclose(fd);
    }
};

} // namespace {}

EREBUS_EXPORT std::string loadTextFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        throw Er::Exception(ER_HERE(), Er::Util::format("Failed to open [%s]", path.c_str()));

    std::stringstream ss;
    ss << file.rdbuf();

    return ss.str();
}

EREBUS_EXPORT Bytes loadBinaryFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
        throw Er::Exception(ER_HERE(), Er::Util::format("Failed to open [%s]", path.c_str()));

    std::string buffer(std::istreambuf_iterator<char>(file), {});

    return Bytes(std::move(buffer));
}

} // namespace Util {}

} // namespace Er {]