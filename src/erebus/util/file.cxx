#include <erebus/exception.hxx>
#include <erebus/util/file.hxx>
#include <erebus/util/format.hxx>

#include <fstream>
#include <sstream>

namespace Er
{

namespace Util
{

EREBUS_EXPORT std::string loadFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        throw Er::Exception(ER_HERE(), Er::Util::format("Failed to open [%s]", path.c_str()));

    std::stringstream ss;
    ss << file.rdbuf();

    return ss.str();
}

} // namespace Util {}

} // namespace Er {]