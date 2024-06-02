#pragma once

#include <erebus-desktop/erebus-desktop.hxx>
#include <erebus-srv/erebus-srv.hxx>

#include <chrono>
#include <mutex>
#include <unordered_map>


namespace Er
{

namespace Desktop
{

namespace Private
{

class Service final
    : public Er::Server::IService
    , public Er::NonCopyable
{
public:
    ~Service();
    explicit Service(Er::Log::ILog* log);

    void registerService(Er::Server::IServiceContainer* container);
    void unregisterService(Er::Server::IServiceContainer* container);

    SessionId allocateSession() override;
    void deleteSession(SessionId id)  override;
    Er::PropertyBag request(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId) override; 
    StreamId beginStream(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId) override;
    void endStream(StreamId id, std::optional<SessionId> sessionId) override;
    Er::PropertyBag next(StreamId id, std::optional<SessionId> sessionId) override;

private:
    struct Session
        : public Er::NonCopyable
    {
        std::chrono::steady_clock::time_point touched = std::chrono::steady_clock::now();
        SessionId id;

        explicit Session(SessionId id) noexcept
            : id(id)
        {}
    };

    enum class StreamType
    {
        Icons
    };

    struct Stream
        : public Er::NonCopyable
    {
        std::chrono::steady_clock::time_point touched = std::chrono::steady_clock::now();
        StreamType type;
        StreamId id;

        explicit Stream(StreamType type, StreamId id) noexcept
            : type(type)
            , id(id)
        {}
    };

    struct IconsStream final
        : public Stream
    {
        explicit IconsStream(StreamId id) noexcept
            : Stream(StreamType::Icons, id)
        {}

        size_t next = 0;
    };

    Session* getSession(std::optional<SessionId> id);
    void dropStaleSessions() noexcept;
    void dropStaleStreams() noexcept;
    Er::PropertyBag queryIcon(const Er::PropertyBag& args, std::optional<SessionId> sessionId);

    const unsigned kSessionTimeoutSeconds = 60 * 60;
    const unsigned kStreamTimeoutSeconds = 60;

    Er::Log::ILog* const m_log;
   
    std::mutex m_mutexSession;
    SessionId m_nextSessionId = 0;
    StreamId m_nextStreamId = 0;
    std::unordered_map<StreamId, std::unique_ptr<Session>> m_sessions;
    std::unordered_map<StreamId, std::unique_ptr<Stream>> m_streams;
};

} // namespace Private {}

} // namespace Desktop {}

} // namespace Er {}
