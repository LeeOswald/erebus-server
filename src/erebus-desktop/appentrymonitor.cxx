#include <erebus-desktop/erebus-desktop.hxx>
#include <erebus/system/pathresolver.hxx>
#include <erebus/system/thread.hxx>
#include <erebus/system/user.hxx>
#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/file.hxx>
#include <erebus/util/inifile.hxx>
#include <erebus/util/stringutil.hxx>

#include <filesystem>
#include <regex>
#include <shared_mutex>
#include <thread>
#include <unordered_map>


namespace Er
{

namespace Desktop
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

        ErAssert(*p == '\"');
        return std::string_view(start, p - start);
    }

    auto p = start;
    while ((p < end) && (*p != ' '))
        ++p;

    return std::string_view(start, p - start);
}


class AppEntryMonitorImpl final
    : public IAppEntryMonitor
    , public Er::NonCopyable
{
public:
    ~AppEntryMonitorImpl()
    {
    }

    explicit AppEntryMonitorImpl(Er::Log::ILog* log)
        : m_log(log)
        , m_worker([this](std::stop_token stop) { worker(stop); }) 
    {
    }

    std::shared_ptr<AppEntry> lookup(const std::string& exe) const override
    {
        std::shared_lock l(m_entriesLock);

        auto it = m_entries.find(exe);
        if (it == m_entries.end())
            return std::shared_ptr<AppEntry>();

        return it->second;
    }

    std::vector<std::shared_ptr<AppEntry>> snapshot() const override
    {
        std::vector<std::shared_ptr<AppEntry>> v;

        std::shared_lock l(m_entriesLock);

        for (auto& e: m_entries)
        {
            v.push_back(e.second);
        }

        return v;
    }

private:
    void worker(std::stop_token stop)
    {
        Er::System::CurrentThread::setName("appentrymonitor");

        addXdgDataDirs(stop);
        addUserDirs(stop);

        std::shared_lock l(m_dirsLock);
        for (auto& dir: m_dirs)
            parseFiles(dir, stop);
    }

    void addXdgDataDirs(std::stop_token stop)
    {
        auto xdgDirs = std::getenv("XDG_DATA_DIRS");
        if (!xdgDirs)
        {
            Er::Log::Warning(m_log, ErLogComponent("AppEntryMonitorImpl")) << "XDG_DATA_DIRS is not set";
            return;
        }

        auto dirs = Er::Util::split(std::string_view(xdgDirs), std::string_view(":"), Er::Util::SplitSkipEmptyParts);
        for (auto& d: dirs)
        {
            if (stop.stop_requested())
                break;

            std::filesystem::path path(d);
            path.append("applications");
            std::error_code ec;
            if (!std::filesystem::exists(path, ec))
                continue;

            auto pathStr = path.string();
            if (::access(pathStr.c_str(), R_OK) == -1)
            {
                Er::Log::Warning(m_log, ErLogComponent("AppEntryMonitorImpl")) << "failed to access " << pathStr;
                continue;
            }

            Er::Log::Debug(m_log, ErLogComponent("AppEntryMonitorImpl")) << "including " << pathStr;

            {
                std::unique_lock l(m_dirsLock);
                m_dirs.push_back(std::move(pathStr));
            }
        }
    }

    void addUserDirs(std::stop_token stop)
    {
        auto users = Er::System::User::enumerate();
        for (auto& u: users)
        {
            if (stop.stop_requested())
                break;

            if (u.homeDir.empty())
                continue;

            if (::access(u.homeDir.c_str(), R_OK) == -1)
            {
                Er::Log::Warning(m_log, ErLogComponent("AppEntryMonitorImpl")) << "failed to access " << u.homeDir;
                continue;
            }

            std::filesystem::path path(u.homeDir);
            path.append(".local/share/applications");
            
            std::error_code ec;
            if (!std::filesystem::exists(path, ec))
                continue;

            auto pathStr = path.string();
            Er::Log::Debug(m_log, ErLogComponent("AppEntryMonitorImpl")) << "including " << pathStr;
            
            {
                std::unique_lock l(m_dirsLock);
                m_dirs.push_back(std::move(pathStr));
            }
        }
    }

    void parseFiles(const std::string& dir, std::stop_token stop)
    {
        std::vector<std::string> filePaths;
        Er::Util::searchFor(
            filePaths, 
            dir, 
            nullptr, 
            false,
            Er::Util::FileSearchMode::FilesOnly, 
            [this, &stop](const std::string& path) 
            { 
                if (!stop.stop_requested() && std::regex_match(path, DesktopFilePattern))
                {
                    Er::Log::Debug(m_log, ErLogComponent("AppEntryMonitorImpl")) << "adding " << path;
                    return true;
                }
                return false;
            }
        );

        for (auto& file: filePaths)
        {
            if (stop.stop_requested())
                break;

            Er::protectedCall<void>(
                m_log, 
                ErLogComponent("AppEntryMonitorImpl"),
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

    std::shared_ptr<AppEntry> parseFile(const std::string& path)
    {
        auto contents = Er::Util::loadTextFile(path);
        auto ini = Er::Util::IniFile::parse(contents);

        auto e = std::make_shared<AppEntry>();
        auto name = Er::Util::IniFile::lookup(ini, std::string_view("Desktop Entry"), std::string_view("Name"));
        if (name)
            e->name = std::move(*name);

        auto exe = Er::Util::IniFile::lookup(ini, std::string_view("Desktop Entry"), std::string_view("Exec"));
        if (!exe)
            return std::shared_ptr<AppEntry>();

        auto exeName = extractExeNameFromCommand(*exe);
        if (exeName.empty())
        {
            Er::Log::Warning(m_log, ErLogComponent("AppEntryMonitorImpl")) << "invalid executable name [" << *exe << "] in " << path;
            return std::shared_ptr<AppEntry>();
        }

        auto exePath = resolveExePath(exeName);
        if (!exePath)
        {
            Er::Log::Warning(m_log, ErLogComponent("AppEntryMonitorImpl")) << "failed to find executable [" << *exe << "] for " << path;
            return std::shared_ptr<AppEntry>();
        }

        e->exec = *exePath;

        auto ico = Er::Util::IniFile::lookup(ini, std::string_view("Desktop Entry"), std::string_view("Icon"));
        if (!ico)
            return std::shared_ptr<AppEntry>();

        e->icon = std::move(*ico);

        Er::Log::Debug(m_log, ErLogComponent("AppEntryMonitorImpl")) << e->exec << " -> " << e->icon;

        return e;
    }

    std::optional<std::string> resolveExePath(std::string_view exe) const
    {
        std::filesystem::path path(exe);
        if (path.is_absolute())
            return std::make_optional(std::string(exe));

        return m_pathResolver.resolve(exe);
    }


    Er::Log::ILog* const m_log;
    std::jthread m_worker; 
    Er::System::PathResolver m_pathResolver;
    mutable std::shared_mutex m_dirsLock;
    std::vector<std::string> m_dirs;
    mutable std::shared_mutex m_entriesLock;
    std::unordered_map<std::string, std::shared_ptr<AppEntry>> m_entries; // exe -> AppEntry
};



} // namespace {}


EREBUSDESKTOP_EXPORT std::shared_ptr<IAppEntryMonitor> createAppEntryMonitor(Er::Log::ILog* log)
{
    return std::make_shared<AppEntryMonitorImpl>(log);
}


} // namespace Desktop {}

} // namespace Er {}