#pragma once

#include <erebus-desktop/erebus-desktop.hxx>

#include <erebus/log.hxx>


namespace Er
{

namespace Desktop
{



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

    virtual bool requestIcon(const IconRequest& request, std::chrono::milliseconds timeout) = 0;
    virtual std::optional<IconRequest> pullIconRequest(std::chrono::milliseconds timeout) = 0;
    virtual bool sendIcon(const IconResponse& response, std::chrono::milliseconds timeout) = 0;
    virtual std::optional<IconResponse> pullIcon(std::chrono::milliseconds timeout) = 0;
};


EREBUSDESKTOP_EXPORT std::shared_ptr<IIconCacheIpc> createIconCacheIpc(const char* queueNameIn, const char* queueNameOut, size_t queueLimit);
EREBUSDESKTOP_EXPORT std::shared_ptr<IIconCacheIpc> openIconCacheIpc(const char* queueNameIn, const char* queueNameOut);

EREBUSDESKTOP_EXPORT std::string makeIconCachePath(const std::string& cacheDir, const std::string& name, unsigned size, std::string_view ext);



} // namespace Desktop {}

} // namespace Er {}

