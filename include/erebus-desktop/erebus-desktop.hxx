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


struct IIconCacheIpc
{
    struct IconRequest
    {
        std::string name; 

        IconRequest() noexcept = default;
        
        IconRequest(std::string_view name)
            : name(name)
        {}
    };

    struct IconResponse
    {
        enum class Result: uint8_t
        {
            Ok,
            NotFound,
            Invalid
        };

        Result result;
        std::string name; // icon name
        std::string path; // cached icon path

        IconResponse() noexcept = default;

        IconResponse(Result result, std::string_view name, std::string_view path)
            : result(result)
            , name(name)
            , path(path)
        {}
    };

    virtual ~IIconCacheIpc() {}

    virtual bool requestIcon(std::string_view name) = 0;
    virtual std::vector<IconRequest> pullIconRequests() = 0;
    virtual bool sendIcon(IconResponse::Result result, std::string_view name, std::string_view path) = 0;
    virtual std::vector<IconResponse> pullIcons() = 0;
};


EREBUSDESKTOP_EXPORT std::shared_ptr<IIconCacheIpc> createIconCacheIpc(const char* queueNameIn, const char* queueNameOut, size_t queueLimit);


} // namespace Desktop {}

} // namespace Er {}
