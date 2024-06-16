#include "desktopfile.hxx"

#include <erebus/util/stringutil.hxx>

#include <fstream>
#include <vector>

namespace Er
{

namespace Desktop
{

namespace Private
{

std::string dekstopFilePathForPid(uint64_t pid)
{
    auto strPid = std::to_string(pid);
    std::string path("/proc/");
    path.append(strPid);
    path.append("/environ");

    std::ifstream file(path, std::ios_base::binary);
    if (!file.is_open())
        return std::string();
    
    std::string desktop;
    std::string gioDesktop;
    std::string launchedPid;

    std::string line;
    std::vector<std::string> v;
    v.reserve(2);

    while (std::getline(file, line, '\0'))
    {
        v.clear();
        Er::Util::split(
            line, 
            std::string_view("="), 
            Er::Util::SplitKeepEmptyParts, 
            [&v](std::string&& part)
            {
                v.push_back(std::move(part));
                return true;
            }
        );

        if (v.size() != 2)
            continue;;

        if (v[0] == "BAMF_DESKTOP_FILE_HINT")
        {
            desktop = v[1];
            break;
        }

        if (v[0] == "GIO_LAUNCHED_DESKTOP_FILE")
        {
            gioDesktop = v[1];
        }
        else if (v[0] == "GIO_LAUNCHED_DESKTOP_FILE_PID")
        {
            launchedPid = v[1];
        }

        if (!gioDesktop.empty() && !launchedPid.empty())
        {
            if (launchedPid == strPid)
            {
                desktop = gioDesktop;
            }

            break;
        }
    }

    return desktop;
}


} // namespace Private {}

} // namespace Desktop {}

} // namespace Er {}