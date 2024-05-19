#include <erebus/util/exceptionutil.hxx>
#include <erebus/util/generichandle.hxx>
#include <erebus/util/sha256.hxx>

#include "iconcache.hxx"

#include <filesystem>
#include <sstream>

#include <boost/process.hpp>

namespace Er
{
    
namespace Private
{

namespace
{

struct FileCloser
{
    void operator()(int fd)
    {
        ::close(fd);
    }
};

} // namespace {}

IconCache::~IconCache()
{
}

IconCache::IconCache(Er::Log::ILog* log, const std::string& iconTheme, const std::string& iconCacheAgent, const std::string& iconCacheDir)
    : m_log(log)
    , m_iconTheme(iconTheme)
    , m_iconCacheAgent(iconCacheAgent)
    , m_iconCacheDir(iconCacheDir)
    , m_workerExited(0)
{
}

std::unordered_map<std::string, std::string> IconCache::lookup(const std::vector<std::string>& iconNames, unsigned size) const
{
    std::unordered_map<std::string, std::string> cachedPaths;

    std::vector<std::string> iconsToRequest;
    iconsToRequest.reserve(iconNames.size());

    // maybe it's already in cache
    for (auto& name: iconNames)
    {
        auto iconPath = makeCachePath(name, size);
        
        std::filesystem::path path(iconPath);
        if (std::filesystem::exists(path))
            cachedPaths.insert({ name, std::move(iconPath) });
        else
            iconsToRequest.push_back(std::move(name));
    }

    // cache what's missing
    if (!iconsToRequest.empty())
    {
        auto ret = callCacheAgent(nullptr, &iconsToRequest, size, nullptr);

        // check if icons have been cached successfully
        for (auto& name: iconsToRequest)
        {
            auto iconPath = makeCachePath(name, size);
            
            std::filesystem::path path(iconPath);
            if (std::filesystem::exists(path))
                cachedPaths.insert({ name, std::move(iconPath) });
            
        }
    }

    return cachedPaths;
}

std::optional<std::string> IconCache::lookup(const std::string& iconName, unsigned size) const
{
    auto iconPath = makeCachePath(iconName, size);
        
    std::filesystem::path path(iconPath);
    if (std::filesystem::exists(path))
        return std::make_optional(std::move(iconPath));

    std::vector<std::string> iconsToRequest{ iconName };
    auto ret = callCacheAgent(nullptr, &iconsToRequest, size, nullptr);
    if (ret == 1)
        return std::make_optional(std::move(iconPath));

    return std::nullopt;
}

void IconCache::prefetch(const std::vector<std::string>& iconNames, unsigned size)
{
    std::lock_guard l(m_mutex);

    if (m_worker)
    {
        if (m_workerExited.load(std::memory_order_acquire) > 0)
        {
            // worker is finishing or has finished
            m_worker.reset();

            m_workerExited.store(0, std::memory_order_release);
        }
        else
        {
            return; // already running
        }
    }

    std::vector<std::string> iconsToRequest;
    iconsToRequest.reserve(iconNames.size());

    for (auto& name: iconNames)
    {
        auto iconPath = makeCachePath(name, size);
        // maybe it's already in cache
        std::filesystem::path path(iconPath);
        if (!std::filesystem::exists(path))
            iconsToRequest.push_back(std::move(name));
    }

    if (!iconsToRequest.empty())
    {
        // pass requested icon names thru a temporary file
        char tempFileName[PATH_MAX] = "/tmp/erebus_cache_agent_XXXXXX";
        {
            Er::Util::GenericHandle<int, int, -1, FileCloser> tempFile(::mkstemp(tempFileName));
            for (auto& ico: iconNames)
            {
                ::write(tempFile, ico.data(), ico.size());
                ::write(tempFile, "\n", 1);
            }
        }

        std::string tmpName(tempFileName);

        m_worker = std::make_unique<std::jthread>(
            [this, tmpName, size](std::stop_token stop)
            {
                callCacheAgent(&tmpName, nullptr, size, &stop);

                m_workerExited.store(1, std::memory_order_release);
            }
        );
    }
}

std::string IconCache::makeCachePath(const std::string& name, unsigned size) const
{
    std::filesystem::path path(m_iconCacheDir);
    auto sz = std::to_string(size);
    if (std::filesystem::path(name).is_absolute())
    {
        // make path like /tmp/iconcache/39534cecc15cd261e9eb3c8cd3544d4f839db599979a9348bf54afc896a25ce8_32x32.png
        Er::Util::Sha256 hash;
        hash.update(name);
        auto hashStr = hash.str(hash.digest());

        std::string fileName(std::move(hashStr));
        fileName.append("_");
        fileName.append(sz);
        fileName.append("x");
        fileName.append(sz);
        fileName.append(".png");

        path.append(fileName);

        return path.string();
    }

    // make path like /tmp/iconcache/mtapp_32x32.png
    std::string fileName(name);
    fileName.append("_");
    fileName.append(sz);
    fileName.append("x");
    fileName.append(sz);
    fileName.append(".png");

    path.append(fileName);

    return path.string();
}

int IconCache::callCacheAgent(const std::string* sourceFile, const std::vector<std::string>* iconNames, unsigned size, std::stop_token* stop) const noexcept
{
    return Er::protectedCall<int>(
        m_log,
        ErLogComponent("IconCache"),
        [this, size, stop, sourceFile, iconNames]()
        {
            std::ostringstream cmd;
            cmd << m_iconCacheAgent << " --cache " << m_iconCacheDir << " --size " << size;
            if (!m_iconTheme.empty()) 
                cmd << " --theme " << m_iconTheme;
            
            if (sourceFile)
            {
                cmd << " --source " << *sourceFile;
            }
            else if (iconNames)
            {
                cmd << " --icons ";
                bool first = true;
                for (auto& name: *iconNames)
                {
                    if (!first)
                        cmd << ":";
                    else
                        first = false;

                    cmd << name;
                }
            }

            boost::process::ipstream outPipe;
            boost::process::ipstream errPipe;
            boost::process::child agent(cmd.str(), boost::process::std_out > outPipe, boost::process::std_err > errPipe);
            
            std::string line;
            while (agent.running()) 
            {
                if (std::getline(outPipe, line) && !line.empty())
                    m_log->write(std::make_shared<Er::Log::Record>(Er::Log::Level::Info, Er::System::Time::gmt(), agent.id(), agent.id(), ErLogComponent("IconCacheAgent"), std::move(line)));
                
                if (std::getline(errPipe, line) && !line.empty())
                    m_log->write(std::make_shared<Er::Log::Record>(Er::Log::Level::Error, Er::System::Time::gmt(), agent.id(), agent.id(), ErLogComponent("IconCacheAgent"), std::move(line)));
                
                if (stop)
                {
                    if (stop->stop_requested())
                    {
                        std::error_code ec;
                        agent.terminate(ec);
                        return -1;
                    }
                }
            }

            while (std::getline(outPipe, line) && !line.empty())
                m_log->write(std::make_shared<Er::Log::Record>(Er::Log::Level::Info, Er::System::Time::gmt(), agent.id(), agent.id(), ErLogComponent("IconCacheAgent"), std::move(line)));
                
            while (std::getline(errPipe, line) && !line.empty())
                m_log->write(std::make_shared<Er::Log::Record>(Er::Log::Level::Error, Er::System::Time::gmt(), agent.id(), agent.id(), ErLogComponent("IconCacheAgent"), std::move(line)));
            

            agent.wait();

            if (sourceFile)
                ::remove(sourceFile->c_str());

            return agent.exit_code();
        }
    );
}


} // namespace Private {}

} // namespace Er {}
