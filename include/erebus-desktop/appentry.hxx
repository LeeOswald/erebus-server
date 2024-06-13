#pragma once

#include <erebus-desktop/erebus-desktop.hxx>

#include <erebus/log.hxx>


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


struct IAppEntryCallback
{
    virtual void appEntryAdded(std::shared_ptr<AppEntry> app) = 0;

protected:
    virtual ~IAppEntryCallback() {};
};


struct IAppEntryMonitor
{
    virtual ~IAppEntryMonitor() {}

    virtual std::shared_ptr<AppEntry> lookup(const std::string& exe) const = 0;
    virtual std::vector<std::shared_ptr<AppEntry>> snapshot() const = 0;
    virtual void registerCallback(IAppEntryCallback* c) = 0;
    virtual void unregisterCallback(IAppEntryCallback* c) = 0;
};


EREBUSDESKTOP_EXPORT std::shared_ptr<IAppEntryMonitor> createAppEntryMonitor(Er::Log::ILog* log);



} // namespace Desktop {}

} // namespace Er {}

