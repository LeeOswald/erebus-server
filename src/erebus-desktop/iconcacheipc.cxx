#include <erebus/exception.hxx>
#include <erebus-desktop/erebus-desktop.hxx>

#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/thread/thread_time.hpp>

namespace Er
{

namespace Desktop
{

namespace
{

class IconCacheIpcImpl final
    : public IIconCacheIpc
    , public Er::NonCopyable
{
public:
    ~IconCacheIpcImpl()
    {
    }

    explicit IconCacheIpcImpl(const char* queueNameIn, const char* queueNameOut, size_t queueLimit)
        : m_queueIn(boost::interprocess::open_or_create, queueNameIn, queueLimit, MessageSize)
        , m_queueOut(boost::interprocess::open_or_create, queueNameOut, queueLimit, MessageSize)
    {}

    bool requestIcon(const IconRequest& request, std::chrono::milliseconds timeout) override
    {
        if (request.name.length() > MaxIconName)
            throw Er::Exception(ER_HERE(), "Icon name too long");

        IconRequestRaw r;
        r.size = request.size;
        std::memcpy(r.name, request.name.data(), request.name.length());
        r.name[request.name.length()] = '\0';
        r.nameLen = request.name.length();

        auto then = toAbsolute(timeout);
        return m_queueOut.timed_send(&r, sizeof(r), 0, then);
    }

    std::optional<IconRequest> pullIconRequest(std::chrono::milliseconds timeout) override
    {
        char buffer[MessageSize];
        auto r = reinterpret_cast<IconRequestRaw*>(&buffer[0]);

        unsigned long rd = 0;
        unsigned int priority = 0;

        auto then = toAbsolute(timeout);
        if (!m_queueIn.timed_receive(r, sizeof(buffer), rd, priority, then))
            return std::nullopt;

        if (rd != sizeof(IconRequestRaw))
            throw Er::Exception(ER_HERE(), "Invalid icon cache request");

        if (r->nameLen > MaxIconName)
            throw Er::Exception(ER_HERE(), "Invalid icon cache request");

        return std::make_optional<IconRequest>(std::string_view(r->name, r->nameLen), uint16_t(r->size));
    }

    bool sendIcon(const IconResponse& response, std::chrono::milliseconds timeout) override
    {
        if (response.request.name.length() > MaxIconName)
            throw Er::Exception(ER_HERE(), "Icon name too long");

        if (response.path.length() > MaxIconPath)
            throw Er::Exception(ER_HERE(), "Icon path too long");

        IconResponseRaw r;
        r.request.size = response.request.size;
        r.result = static_cast<uint8_t>(response.result);        
        
        std::memcpy(r.request.name, response.request.name.data(), response.request.name.length());
        r.request.name[response.request.name.length()] = '\0';
        r.request.nameLen = response.request.name.length();

        std::memcpy(r.path, response.path.data(), response.path.length());
        r.path[response.path.length()] = '\0';
        r.pathLen = response.path.length();

        auto then = toAbsolute(timeout);
        return m_queueOut.timed_send(&r, sizeof(r), 0, then);
    }

    std::optional<IconResponse> pullIcon(std::chrono::milliseconds timeout) override
    {
        char buffer[MessageSize];
        auto r = reinterpret_cast<IconResponseRaw*>(&buffer[0]);
        unsigned long rd = 0;
        unsigned int priority = 0;
        
        auto then = toAbsolute(timeout);

        if (!m_queueIn.timed_receive(r, sizeof(buffer), rd, priority, then))
            return std::nullopt;

        if (rd != sizeof(IconResponseRaw))
            throw Er::Exception(ER_HERE(), "Invalid icon cache response");

        if (r->request.nameLen > MaxIconName)
            throw Er::Exception(ER_HERE(), "Invalid icon cache response");

        if (r->pathLen > MaxIconName)
            throw Er::Exception(ER_HERE(), "Invalid icon cache response");

        return std::make_optional<IconResponse>(std::string_view(r->request.name, r->request.nameLen), uint16_t(r->request.size), static_cast<IconResponse::Result>(r->result), std::string_view(r->path, r->pathLen));
    }

private:
    static boost::posix_time::ptime toAbsolute(std::chrono::milliseconds milliseconds)
    {
        return boost::posix_time::ptime(boost::get_system_time() + boost::posix_time::milliseconds(milliseconds.count()));
    }

    static constexpr size_t MaxIconName = 256;
    static constexpr size_t MaxIconPath = 260;

    struct __attribute__((__packed__)) IconRequestRaw
    {
        uint16_t size;
        uint16_t nameLen;
        char name[MaxIconName + 1];
    };

    struct __attribute__((__packed__)) IconResponseRaw
    {
        IconRequestRaw request;
        uint8_t result;
        uint16_t pathLen;
        char path[MaxIconPath + 1];
    };

    static constexpr size_t MessageSize = sizeof(IconResponseRaw);

    boost::interprocess::message_queue m_queueIn;
    boost::interprocess::message_queue m_queueOut;
};


} // namespace {}


EREBUSDESKTOP_EXPORT std::shared_ptr<IIconCacheIpc> createIconCacheIpc(const char* queueNameIn, const char* queueNameOut, size_t queueLimit)
{
    return std::make_shared<IconCacheIpcImpl>(queueNameIn, queueNameOut, queueLimit);
}


} // namespace Desktop {}

} // namespace Er {}
