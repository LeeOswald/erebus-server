#include <erebus/system/user.hxx>
#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/file.hxx>
#include <erebus/util/inifile.hxx>
#include <erebus/util/stringutil.hxx>

#include "desktopentries.hxx"

#include <filesystem>
#include <regex>


namespace Er
{

namespace Private
{

namespace
{

const std::regex DesktopFilePattern(".*\\.desktop$");

std::string_view extractExeNameFromCommand(std::string_view command) noexcept
{
    auto start = command.data();
    auto end = start + command.length();
    while ((start < end) && std::isspace(*start))
        ++start;

    if (start == end)
        return std::string_view();

    if (*start == '\"') // exe path is quoted
    {
        ++start;
        auto p = start;
        while ((p < end) && (*p != '\"'))
            ++p;

        if (p == end)
            return std::string_view();

        assert(*p == '\"');
        return std::string_view(start, p - start);
    }

    auto p = start;
    while ((p < end) && (*p != ' '))
        ++p;

    return std::string_view(start, p - start);
}

} // namespace {}


DesktopEntries::DesktopEntries(Er::Log::ILog* log)
    : m_log(log)
{
    addXdgDataDirs();
    addUserDirs();

    std::shared_lock l(m_dirsLock);
    for (auto& dir: m_dirs)
        parseFiles(dir);
}

std::shared_ptr<DesktopEntries::Entry> DesktopEntries::lookup(const std::string& exec) const noexcept
{
    std::shared_lock l(m_entriesLock);

    auto it = m_entries.find(exec);
    if (it == m_entries.end())
        return std::shared_ptr<DesktopEntries::Entry>();

    return it->second;
}

std::vector<std::string> DesktopEntries::iconList() const
{
    std::vector<std::string> v;

    std::shared_lock l(m_entriesLock);

    for (auto& e: m_entries)
    {
        v.push_back(e.second->icon);
    }

    return v;
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
            std::unique_lock l(m_dirsLock);
            m_dirs.push_back(std::move(pathStr));
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
            std::unique_lock l(m_dirsLock);
            m_dirs.push_back(std::move(pathStr));
        }
    }
}

void DesktopEntries::parseFiles(const std::string& dir)
{
    std::vector<std::string> filePaths;
    Er::Util::searchFor(
        filePaths, 
        dir, 
        nullptr, 
        false,
        Er::Util::FileSearchMode::FilesOnly, 
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

    for (auto& file: filePaths)
    {
        Er::protectedCall<void>(
            m_log, 
            LogComponent("DesktopEntries"),
            [this](const std::string& file)
            {
                auto result = parseFile(file);
                if (result)
                {
                    std::unique_lock l(m_entriesLock);
                    m_entries.insert({ result->exec, result });
                }
            },
            file
        );
    }
}

std::shared_ptr<DesktopEntries::Entry> DesktopEntries::parseFile(const std::string& path)
{
    auto contents = Er::Util::loadTextFile(path);
    auto ini = Er::Util::IniFile::parse(contents);

    auto e = std::make_shared<Entry>();
    auto name = Er::Util::IniFile::lookup(ini, std::string_view("Desktop Entry"), std::string_view("Name"));
    if (name)
        e->name = std::move(*name);

    auto exe = Er::Util::IniFile::lookup(ini, std::string_view("Desktop Entry"), std::string_view("Exec"));
    if (!exe)
        return std::shared_ptr<DesktopEntries::Entry>();

    auto exeName = extractExeNameFromCommand(*exe);
    if (exeName.empty())
    {
        Er::Log::Warning(m_log, LogComponent("DesktopEntries")) << "invalid executable name [" << *exe << "] in " << path;
        return std::shared_ptr<DesktopEntries::Entry>();
    }

    auto exePath = resolveExePath(exeName);
    if (!exePath)
    {
        Er::Log::Warning(m_log, LogComponent("DesktopEntries")) << "failed to find executable [" << *exe << "] for " << path;
        return std::shared_ptr<DesktopEntries::Entry>();
    }

    e->exec = *exePath;

    auto ico = Er::Util::IniFile::lookup(ini, std::string_view("Desktop Entry"), std::string_view("Icon"));
    if (!ico)
        return std::shared_ptr<DesktopEntries::Entry>();

    e->icon = std::move(*ico);

    Er::Log::Debug(m_log, LogComponent("DesktopEntries")) << e->exec << " -> " << e->icon;

    return e;
}

std::optional<std::string> DesktopEntries::resolveExePath(std::string_view exe) const
{
    std::filesystem::path path(exe);
    if (path.is_absolute())
        return std::make_optional(std::string(exe));

    return m_pathResolver.resolve(exe);
}

} // Private {}

} // namespace Er {}