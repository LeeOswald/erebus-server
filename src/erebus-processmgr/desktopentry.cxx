#include <erebus/system/user.hxx>
#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/file.hxx>
#include <erebus/util/inifile.hxx>
#include <erebus/util/stringutil.hxx>
#include <erebus-processmgr/desktopentry.hxx>

#include <filesystem>
#include <regex>


namespace Er
{

namespace DesktopEnv
{

namespace
{

const std::regex DesktopFilePattern(".*\\.desktop$");

template <typename PredicateT>
void searchFor(
    std::unordered_set<std::string>* results, 
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
                    results->emplace(std::move(path));
            }
        }
    }
}

} // namespace {}


DesktopEntries::DesktopEntries(Er::Log::ILog* log)
    : m_log(log)
{
    addXdgDataDirs();
    addUserDirs();
    for (auto& dir: m_dirs)
        enumerateFiles(dir);

    parseFiles();
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
        
        {
            std::unique_lock l(m_mutex);
            m_dirs.emplace(std::move(pathStr));
        }
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
        
        {
            std::unique_lock l(m_mutex);
            m_dirs.emplace(std::move(pathStr));
        }
    }
}

void DesktopEntries::enumerateFiles(const std::string& dir)
{
    searchFor(
        &m_files, 
        nullptr, 
        dir, 
        nullptr, 
        false, 
        [this](const std::string& path) 
        { 
            if (std::regex_match(path, DesktopFilePattern))
            {
                Er::Log::Debug(m_log, LogComponent("DesktopEntries")) << "adding " << path;
                return true;
            }
            return false;
        }
    );
}

void DesktopEntries::parseFiles()
{
    for (auto& file: m_files)
    {
        Er::protectedCall<void>(
            m_log, 
            LogComponent("DesktopEntries"),
            [this](const std::string& file)
            {
                auto result = parseFile(file);
                if (result)
                    m_entries.insert({ result->exec, std::move(*result) });
            },
            file
        );
    }
}

std::optional<DesktopEntries::Entry> DesktopEntries::parseFile(const std::string& path)
{
    auto contents = Er::Util::loadFile(path);
    auto ini = Er::Util::IniFile::parse(contents);

    Entry e;
    auto name = Er::Util::IniFile::lookup(ini, std::string_view("Desktop Entry"), std::string_view("Name"));
    if (name)
        e.name = std::move(*name);

    auto exe = Er::Util::IniFile::lookup(ini, std::string_view("Desktop Entry"), std::string_view("Exec"));
    if (!exe)
        return std::nullopt;

    e.exec = std::move(*exe);

    auto ico = Er::Util::IniFile::lookup(ini, std::string_view("Desktop Entry"), std::string_view("Icon"));
    if (!ico)
        return std::nullopt;

    e.icon = std::move(*ico);

    Er::Log::Debug(m_log, LogComponent("DesktopEntries")) << e.exec << " -> " << e.icon;

    return std::make_optional<Entry>(std::move(e));
}

} // DesktopEnv {}

} // namespace Er {}