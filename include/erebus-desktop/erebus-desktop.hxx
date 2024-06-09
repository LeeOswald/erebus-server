#pragma once

#include <erebus/knownprops.hxx>
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


struct IIconCacheIpc
{
    struct IconRequest
    {
        std::string name;
        uint16_t size; 

        IconRequest(std::string_view name, uint16_t size)
            : name(name)
            , size(size)
        {}
    };

    struct IconResponse
    {
        enum class Result: uint8_t
        {
            Ok,
            NotFound
        };

        IconRequest request;
        Result result;
        std::string path; // cached icon path

        IconResponse(std::string_view name, uint16_t size, Result result) noexcept
            : request(name, size)
            , result(result)
        {}

        IconResponse(std::string_view name, uint16_t size, Result result, std::string_view path)
            : request(name, size)
            , result(result)
            , path(path)
        {}
    };

    virtual ~IIconCacheIpc() {}

    virtual bool requestIcon(const IconRequest& request, std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) = 0;
    virtual std::optional<IconRequest> pullIconRequest(std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) = 0;
    virtual bool sendIcon(const IconResponse& response, std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) = 0;
    virtual std::optional<IconResponse> pullIcon(std::chrono::milliseconds timeout = std::chrono::milliseconds(0)) = 0;
};


EREBUSDESKTOP_EXPORT std::shared_ptr<IIconCacheIpc> createIconCacheIpc(const char* queueNameIn, const char* queueNameOut, size_t queueLimit);

EREBUSDESKTOP_EXPORT std::string makeIconCachePath(const std::string& cacheDir, const std::string& name, unsigned size, std::string_view ext);


namespace Props
{

struct IconFormatter
{
    void operator()(const Bytes& ico, std::ostream& s) 
    { 
        if (ico.empty())
            s << "[null icon]";
        else
            s << "[icon (" << ico.size() << " bytes)]";
    }
};    


using Icon = PropertyValue<Bytes, ER_PROPID("app.icon.png"), "Icon Bytes", PropertyComparator<Bytes>, IconFormatter>;
using IconName = PropertyValue<std::string, ER_PROPID("app.icon.name"), "Icon Name">;
using IconSize = PropertyValue<uint32_t, ER_PROPID("app.icon.size"), "Icon Size">;
using Comm = PropertyValue<std::string, ER_PROPID("app.comm"), "Process Name">;
using Exe = PropertyValue<std::string, ER_PROPID("app.exe"), "Executable Path">;
using Pid = PropertyValue<uint64_t, ER_PROPID("app.pid"), "PID">;

namespace Private
{

inline void registerAll(Er::Log::ILog* log)
{
    registerProperty(std::make_shared<PropertyInfoWrapper<Icon>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<IconName>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<IconSize>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<Comm>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<Exe>>(), log);
    registerProperty(std::make_shared<PropertyInfoWrapper<Pid>>(), log);
}

inline void unregisterAll(Er::Log::ILog* log)
{
    unregisterProperty(lookupProperty(Icon::Id::value), log);
    unregisterProperty(lookupProperty(IconName::Id::value), log);
    unregisterProperty(lookupProperty(IconSize::Id::value), log);
    unregisterProperty(lookupProperty(Comm::Id::value), log);
    unregisterProperty(lookupProperty(Exe::Id::value), log);
    unregisterProperty(lookupProperty(Pid::Id::value), log);
}


} // namespace Private {}

} // namespace Props {}


namespace Requests
{

static const std::string_view QueryIcon = "QueryIcon";
static const std::string_view PullIcons = "PullIcons";


} // namespace Requests {}

} // namespace Desktop {}

} // namespace Er {}
