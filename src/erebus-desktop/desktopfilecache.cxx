#include "desktopfilecache.hxx"

#include <erebus/system/thread.hxx>
#include <erebus/system/user.hxx>
#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/file.hxx>
#include  <erebus/util/inifile.hxx>
#include <erebus/util/stringutil.hxx>

#include <filesystem>
#include <fstream>
#include <regex>
#include <vector>


#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/process/search_path.hpp>


namespace Er
{

namespace Desktop
{

namespace Private
{

namespace
{

const std::regex DesktopFilePattern(".*\\.desktop$");

std::string_view skipws(std::string_view str)
{
    auto start = str.data();
    auto end = start + str.length();
    while ((start < end) && std::isspace(*start))
        ++start;

    if (start == end)
        return std::string_view();

    return std::string_view(start, end - start);
}

std::string_view unquote(std::string_view str)
{
    if (str.empty())
        return std::string_view();

    char q = 0;
    if (str[0] == '\'' || str[0] == '\"')
        q = str[0];
    else
        return str;

    size_t i = 1;
    while (i < str.length())
    {
        if (str[i] == q)
            return std::string_view(str.data() + 1, i - 1);

        ++i;
    }

    return std::string_view();
}

std::string extractExeNameFromCommand(std::string_view command) noexcept
{
    std::vector<std::string> args;
    args.reserve(10);
    Er::Util::split2(command, std::string_view(" "), Er::Util::SplitSkipEmptyParts, args);
    if (args.empty())
        return std::string();

    bool env = false;
    for (auto& arg: args)
    {
        auto a = skipws(unquote(arg));
        if (!a.empty())
        {
            if (a.find('=') != a.npos)
            {
                // env var
            }
            else if ((a == "env") && !env)
            {
                // env command
                env = true;
            }
            else
            {
                // actual exe
                return std::string(a);
            }
        }
    }

    return std::string();
}

} // namespace {}


DesktopFileCache::DesktopFileCache(Er::Log::ILog* log)
    : m_searchPaths(boost::this_process::path())
    , m_log(log)
    , m_worker([this](std::stop_token stop) { worker(stop); }) 
{
}

void DesktopFileCache::addXdgDataDirs(const std::string& packed)
{
    std::vector<std::string> dirs;
    Er::Util::split2(std::string_view(packed), std::string_view(":"), Er::Util::SplitSkipEmptyParts, dirs);
    if (dirs.empty())
        return;

    for (auto& dir: dirs)
    {
        std::filesystem::path path(dir);
        path.append("applications");
        
        std::error_code ec;
        if (!std::filesystem::exists(path, ec))
            continue;

        auto pathStr = path.string();
        if (::access(pathStr.c_str(), R_OK) == -1)
        {
            Er::Log::Warning(m_log, ErLogComponent("DesktopFileCache")) << "failed to access " << pathStr;
            continue;
        }

        {
            std::unique_lock l(m_dirsLock);
            m_dirs.push(std::move(dir));
            m_idle = false;
        }
    }

    m_dirsCv.notify_one();
}

bool DesktopFileCache::waitIdle(std::chrono::milliseconds timeout)
{
    std::unique_lock l(m_dirsLock);
    return m_dirsCv.wait_for(l, timeout, [this]() { return m_idle; });
}

void DesktopFileCache::addUserDirs(std::stop_token stop)
{
    auto users = Er::System::User::enumerate();
    for (auto& u: users)
    {
        if (stop.stop_requested())
            break;

        if (u.homeDir.empty())
            continue;

        std::filesystem::path path(u.homeDir);
        path.append(".local/share/applications");
        
        std::error_code ec;
        if (!std::filesystem::exists(path, ec))
            continue;

        auto pathStr = path.string();
        if (::access(pathStr.c_str(), R_OK) == -1)
        {
            Er::Log::Warning(m_log, ErLogComponent("DesktopFileCache")) << "failed to access " << pathStr;
            continue;
        }

        parseFiles(pathStr, stop);
    }
}

void DesktopFileCache::worker(std::stop_token stop)
{
    Er::System::CurrentThread::setName("DesktopFileCache");

    addUserDirs(stop);

    while (!stop.stop_requested())
    {
        std::unique_lock l(m_dirsLock);
        if (!m_dirsCv.wait(l, stop, [this]() { return !m_dirs.empty(); }))
            continue;

        while (!m_dirs.empty() && !stop.stop_requested())
        {
            parseFiles(m_dirs.front(), stop);
            m_dirs.pop();
        }

        m_idle = true;
    }
}

void DesktopFileCache::parseFiles(const std::string& dir, std::stop_token stop)
{
    ErLogDebug(m_log, ErLogComponent("DesktopFileCache"), "Including [%s]", dir.c_str());

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
                Er::Log::Debug(m_log, ErLogComponent("DesktopFileCache")) << "adding " << path;
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
            ErLogComponent("DesktopFileCache"),
            [this](const std::string& filePath)
            {
                auto parsed = parseFile(filePath);
                if (parsed)
                {
                    std::unique_lock l(m_entriesLock);
                    auto existing = m_entriesByExec.find(parsed->real_exec);
                    if (existing != m_entriesByExec.end())
                        m_entriesByExec.erase(existing);

                    m_entriesByExec.insert({ parsed->real_exec, parsed });
                    m_entriesByPath.insert({ filePath, parsed });
                }
            },
            file
        );
    }
}

DesktopFile::Ptr DesktopFileCache::parseFile(const std::string& filePath) const
{
    boost::iostreams::mapped_file_source file(filePath);
    if (!file.is_open())
    {
        Er::Log::Warning(m_log, ErLogComponent("DesktopFileCache")) << "Failed to open [" << filePath << "]";
        return DesktopFile::Ptr();
    }
    
    auto ini = Er::Util::IniFile::parse(std::string_view(file.data(), file.size()));

    auto e = std::make_shared<DesktopFile>();
    auto name = Er::Util::IniFile::lookup(ini, std::string_view("Desktop Entry"), std::string_view("Name"));
    if (name)
        e->name = *name;

    auto exec = Er::Util::IniFile::lookup(ini, std::string_view("Desktop Entry"), std::string_view("Exec"));
    if (!exec) 
    {
        Er::Log::Warning(m_log, ErLogComponent("DesktopFileCache")) << "No Exec field in " << filePath;
        return std::shared_ptr<DesktopFile>();
    }

    e->exec = *exec;

    auto exeName = extractExeNameFromCommand(*exec);
    if (exeName.empty())
    {
        Er::Log::Warning(m_log, ErLogComponent("DesktopFileCache")) << "Unable to extract executable name from [" << *exec << "] in " << filePath;
        return std::shared_ptr<DesktopFile>();
    }

    auto exePath = findExePath(exeName);
    if (!exePath)
    {
        Er::Log::Warning(m_log, ErLogComponent("DesktopFileCache")) << "Failed to find executable [" << *exec << "] for " << filePath;
        return std::shared_ptr<DesktopFile>();
    }

    Er::Log::Debug(m_log, ErLogComponent("DesktopFileCache")) << "Found executable [" << exeName << "] -> [" << *exePath << "]";

    auto realExec = Er::Util::resolveSymlink(*exePath);
    if (realExec)
    {
        e->real_exec = *realExec;
        if (e->real_exec != *exePath)
            Er::Log::Debug(m_log, ErLogComponent("DesktopFileCache")) << "Resolved [" << *exePath << "] -> [" << *realExec << "]";
    }
    else
    {
        e->real_exec = *exePath;
    }

    auto icon = Er::Util::IniFile::lookup(ini, std::string_view("Desktop Entry"), std::string_view("Icon"));
    if (!icon)
        return std::shared_ptr<DesktopFile>();

    e->icon = *icon;

    Er::Log::Info(m_log, ErLogComponent("DesktopFileCache")) << "Icon [" << e->real_exec << "] -> [" << e->icon << "]";

    return e;
}

std::optional<std::string> DesktopFileCache::findExePath(const std::string& exe) const
{
    boost::filesystem::path filename(exe);
    if (filename.is_absolute())
    {
        return std::make_optional(exe);
    }

    auto path = boost::process::search_path(filename, m_searchPaths);
    if (path.empty())
        return std::nullopt;

    return std::make_optional(path.native());
}

DesktopFile::Ptr DesktopFileCache::lookupByExec(const std::string& exec)
{
    std::unique_lock l(m_entriesLock);

    auto it = m_entriesByExec.find(exec);
    if (it == m_entriesByExec.end())
        return DesktopFile::Ptr();

    return it->second;
}

DesktopFile::Ptr DesktopFileCache::lookupByPath(const std::string& path)
{
    // look in the cache
    {
        std::unique_lock l(m_entriesLock);

        auto it = m_entriesByPath.find(path);
        if (it != m_entriesByPath.end())
            return it->second;
    }

    // try loading from file path
    auto desktopFile = Er::protectedCall<DesktopFile::Ptr>(
        m_log, 
        ErLogComponent("DesktopFileCache"),
        [this](const std::string& filePath)
        {
            auto parsed = parseFile(filePath);
            if (parsed)
            {
                std::unique_lock l(m_entriesLock);
                auto existing = m_entriesByExec.find(parsed->real_exec);
                if (existing != m_entriesByExec.end())
                    m_entriesByExec.erase(existing);

                m_entriesByExec.insert({ parsed->real_exec, parsed });
                m_entriesByPath.insert({ filePath, parsed });
            }

            return parsed;
        },
        path
    );

    return desktopFile;
}


} // namespace Private {}

} // namespace Desktop {}

} // namespace Er {}