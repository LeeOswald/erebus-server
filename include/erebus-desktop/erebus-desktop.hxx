#pragma once

#include <erebus/erebus.hxx>
#include <erebus/log.hxx>

#include <vector>

#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef EREBUSDESKTOP_EXPORTS
        #define EREBUSDESKTOP_EXPORT __declspec(dllexport)
    #else
        #define EREBUSDESKTOP_EXPORT __declspec(dllimport)
    #endif
#else
    #define EREBUSDESKTOP_EXPORT __attribute__((visibility("default")))
#endif


namespace Er
{

namespace Desktop
{

struct AppEntry
{
    std::string name;
    std::string exec;
    std::string icon;
};


struct IAppEntryMonitor
{
    virtual ~IAppEntryMonitor() {}

    virtual std::shared_ptr<AppEntry> lookup(const std::string& exe) const = 0;
    virtual std::vector<std::shared_ptr<AppEntry>> snapshot() const = 0;
};


EREBUSDESKTOP_EXPORT std::shared_ptr<IAppEntryMonitor> createAppEntryMonitor(Er::Log::ILog* log);


} // namespace Desktop {}

} // namespace Er {}
