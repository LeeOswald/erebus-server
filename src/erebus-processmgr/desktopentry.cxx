#include <erebus/system/user.hxx>
#include <erebus/util/stringutil.hxx>
#include <erebus-processmgr/desktopentry.hxx>

#include <filesystem>
#include <regex>
#include <unordered_set>

namespace Er
{

namespace DesktopEnv
{

namespace
{

const std::regex DesktopFilePattern(".*\\.desktop$");

template <typename PredicateT>
void searchFor(
    std::vector<std::string>* results, 
    std::unordered_set<std::string>* uniqueDirs, 
    std::string_view dir, 
    const std::unordered_set<std::string>* excludeDirs, 
    bool recursive,
    PredicateT pred
    )
{
    std::error_code ec;
    for (auto& d: std::filesystem::directory_iterator(dir, ec))
    {
        if (ec)
            continue;

        auto path = d.path().string();
        
        if (excludeDirs && (excludeDirs->find(path) != excludeDirs->end()))
            continue;

        if (d.is_symlink(ec) || ec)
            continue;

        if (d.is_directory(ec) && !ec)
        {
            if (recursive)
                searchFor(results, uniqueDirs, path, excludeDirs, recursive, pred);
        }
        else if (d.is_regular_file(ec) && !ec)
        {
            if (pred(path))
            {
                if (uniqueDirs)
                    uniqueDirs->emplace(dir);

                if (results)
                    results->emplace_back(std::move(path));
            }
        }
    }
}

std::vector<std::string> enumerateDesktopEntryFiles(const std::string& dir)
{
    std::vector<std::string> files;
    searchFor(&files, nullptr, dir, nullptr, false, [](const std::string& path) { return std::regex_match(path, DesktopFilePattern); });
    return files;
}

} // namespace {}


DesktopEntries::DesktopEntries(Er::Log::ILog* log)
    : m_log(log)
{
    addXdgDataDirs();
    addUserDirs();
}

void DesktopEntries::addXdgDataDirs()
{
    auto xdgDirs = std::getenv("XDG_DATA_DIRS");
    if (!xdgDirs)
    {
        Er::Log::Warning(m_log, LogComponent("DesktopEntries")) << "XDG_DATA_DIRS is not set";
        return;
    }

    auto dirs = Er::Util::split(std::string_view(xdgDirs), std::string_view(":"), Er::Util::SplitSkipEmptyParts);
    for (auto& d: dirs)
    {
        std::filesystem::path path(d);
        path.append("applications");
        std::error_code ec;
        if (!std::filesystem::exists(path, ec))
            continue;

        auto pathStr = path.string();
        if (::access(pathStr.c_str(), R_OK) == -1)
        {
            Er::Log::Warning(m_log, LogComponent("DesktopEntries")) << "failed to access " << pathStr;
            continue;
        }

        Er::Log::Debug(m_log, LogComponent("DesktopEntries")) << "including " << pathStr;
        m_dirs.emplace_back(std::move(pathStr));
    }
}

void DesktopEntries::addUserDirs()
{
    auto users = Er::System::User::enumerate();
    for (auto& u: users)
    {
        if (u.homeDir.empty())
            continue;

        if (::access(u.homeDir.c_str(), R_OK) == -1)
        {
            Er::Log::Warning(m_log, LogComponent("DesktopEntries")) << "failed to access " << u.homeDir;
            continue;
        }

        std::filesystem::path path(u.homeDir);
        path.append(".local/share/applications");
        
        std::error_code ec;
        if (!std::filesystem::exists(path, ec))
            continue;

        auto pathStr = path.string();
        Er::Log::Debug(m_log, LogComponent("DesktopEntries")) << "including " << pathStr;
        m_dirs.push_back(std::move(pathStr));
    }
}

} // DesktopEnv {}

} // namespace Er {}