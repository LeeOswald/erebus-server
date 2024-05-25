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
        : m_queueIn(boost::interprocess::open_or_create, queueNameIn, queueLimit, std::max(sizeof(IconRequestRaw), sizeof(IconResponseRaw)))
        , m_queueOut(boost::interprocess::open_or_create, queueNameOut, queueLimit, std::max(sizeof(IconRequestRaw), sizeof(IconResponseRaw)))
    {}

    bool requestIcon(std::string_view name, std::chrono::milliseconds timeout) override
    {
        if (name.length() > MaxIconName)
            throw Er::Exception(ER_HERE(), "Icon name too long");

        IconRequestRaw r;
        std::memcpy(r.name, name.data(), name.length());
        r.name[name.length()] = '\0';
        r.nameLen = name.length();

        auto then = toAbsolute(timeout);
        return m_queueOut.timed_send(&r, sizeof(r), 0, then);
    }

    std::optional<IconRequest> pullIconRequest(std::chrono::milliseconds timeout) override
    {
        IconRequestRaw r;
        unsigned long rd = 0;
        unsigned int priority = 0;

        auto then = toAbsolute(timeout);
        if (!m_queueIn.timed_receive(&r, sizeof(r), rd, priority, then))
            return std::nullopt;

        if (rd != sizeof(IconRequestRaw))
            throw Er::Exception(ER_HERE(), "Invalid icon cache request");

        if (r.nameLen > MaxIconName)
            throw Er::Exception(ER_HERE(), "Invalid icon cache request");

        return std::make_optional<IconRequest>(std::string_view(r.name, r.nameLen));
    }

    bool sendIcon(IconResponse::Result result, std::string_view name, std::string_view path, std::chrono::milliseconds timeout) override
    {
        if (name.length() > MaxIconName)
            throw Er::Exception(ER_HERE(), "Icon name too long");

        if (path.length() > MaxIconPath)
            throw Er::Exception(ER_HERE(), "Icon path too long");

        IconResponseRaw r;
        r.result = static_cast<uint8_t>(result);        
        
        std::memcpy(r.name, name.data(), name.length());
        r.name[name.length()] = '\0';
        r.nameLen = name.length();

        std::memcpy(r.path, path.data(), path.length());
        r.path[path.length()] = '\0';
        r.pathLen = path.length();

        auto then = toAbsolute(timeout);
        return m_queueOut.timed_send(&r, sizeof(r), 0, then);
    }

    std::optional<IconResponse> pullIcon(std::chrono::milliseconds timeout) override
    {
        IconResponseRaw r;
        unsigned long rd = 0;
        unsigned int priority = 0;
        
        auto then = toAbsolute(timeout);

        if (!m_queueIn.timed_receive(&r, sizeof(r), rd, priority, then))
            return std::nullopt;

        if (rd != sizeof(IconResponseRaw))
            throw Er::Exception(ER_HERE(), "Invalid icon cache response");

        if (r.nameLen > MaxIconName)
            throw Er::Exception(ER_HERE(), "Invalid icon cache response");

        if (r.pathLen > MaxIconName)
            throw Er::Exception(ER_HERE(), "Invalid icon cache response");

        return std::make_optional<IconResponse>(static_cast<IconResponse::Result>(r.result), std::string_view(r.name, r.nameLen), std::string_view(r.path, r.pathLen));
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
        uint16_t nameLen;
        char name[MaxIconName + 1];
    };

    struct __attribute__((__packed__)) IconResponseRaw
    {
        uint8_t result;
        uint16_t nameLen;
        char name[MaxIconName + 1];
        uint16_t pathLen;
        char path[MaxIconPath + 1];
    };

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
