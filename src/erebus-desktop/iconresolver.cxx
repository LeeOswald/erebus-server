#include "iconresolver.hxx"

#include <regex>

namespace Er
{

namespace Desktop
{

namespace Private
{

namespace
{

struct KnownApp
{
    std::regex command;
    std::string icon;
};

const KnownApp g_KnownApps[] =
{
    { std::regex("^((ba|z|tc|c|k)?sh)$"), "utilities-terminal" }
};

std::string defaultExeIcon()
{
    return std::string("application-x-executable");
}

std::string knownAppIcon(const std::string& comm)
{
    for (auto& k: g_KnownApps)
    {
        if (regex_match(comm, k.command))
            return k.icon;
    }

    return std::string();
}


} // namespace {}


IconResolver::~IconResolver()
{
    m_appEntryMonitor->unregisterCallback(this);
}

IconResolver::IconResolver(Er::Log::ILog* log, std::shared_ptr<Er::Desktop::IAppEntryMonitor> appEntryMonitor)
    : m_log(log)
    , m_appEntryMonitor(appEntryMonitor)
{
    m_appEntryMonitor->registerCallback(this);
}

void IconResolver::appEntryAdded(std::shared_ptr<Er::Desktop::AppEntry> app)
{
}

std::string IconResolver::lookup(std::optional<std::string> exe, std::optional<std::string> comm, std::optional<uint64_t> pid)
{
    if (exe)
    {
        auto app = m_appEntryMonitor->lookup(*exe);
        if (app)
        {
            ErLogDebug(m_log, ErLogComponent("IconResolver"), "[%s] -> [%s]", exe->c_str(), app->icon.c_str());
            return app->icon;
        }
    }

    if (comm)
    {
        auto ico = knownAppIcon(*comm);
        if (!ico.empty())
        {
            ErLogDebug(m_log, ErLogComponent("IconResolver"), "[%s] -> [%s]", exe->c_str(), ico.c_str());
            return ico;
        }
    }

    auto defIcon = defaultExeIcon();
    ErLogDebug(m_log, ErLogComponent("IconResolver"), "[%s] -> [%s]", exe->c_str(), defIcon.c_str());
    return defIcon;
}

} // namespace Private {}

} // namespace Desktop {}

} // namespace Er {}