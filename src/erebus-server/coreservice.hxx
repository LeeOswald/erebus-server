#pragma once

#include <erebus-srv/erebus-srv.hxx>

#include <chrono>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace Er
{

namespace Private
{

class CoreService
    : public Er::Server::IService
    , public Er::NonCopyable
{
public:
    ~CoreService();
    explicit CoreService(Er::Log::ILog* log, Er::Server::Private::IUserDb* userDb);

    void registerService(Er::Server::IServiceContainer* container);
    void unregisterService(Er::Server::IServiceContainer* container);

    SessionId allocateSession() override;
    void deleteSession(SessionId id)  override;
    Er::PropertyBag request(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId) override;
    StreamId beginStream(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId) override;
    void endStream(StreamId id, std::optional<SessionId> sessionId) override;
    Er::PropertyBag next(StreamId id, std::optional<SessionId> sessionId) override;

private:
    enum class StreamType
    {
        UserList
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

    struct UserListStream
        : public Stream
    {
        std::vector<Er::Server::Private::User> users;
        std::size_t next = 0;

        explicit UserListStream(StreamId id, std::vector<Er::Server::Private::User>&& users) noexcept
            : Stream(StreamType::UserList, id)
            , users(std::move(users))
        {}
    };

    Er::PropertyBag getVersion(const Er::PropertyBag& args);
    Er::PropertyBag addUser(const Er::PropertyBag& args);
    Er::PropertyBag removeUser(const Er::PropertyBag& args);
    StreamId beginUserListStream();
    Er::PropertyBag nextUser(UserListStream* stream);

    const unsigned kStreamTimeoutSeconds = 60;

    void dropStaleStreams() noexcept;

    Er::Log::ILog* m_log;
    Er::Server::Private::IUserDb* m_userDb;
    std::mutex m_mutex;
    StreamId m_nextStreamId = 0;
    std::unordered_map<StreamId, std::unique_ptr<Stream>> m_streams;
};


} // namespace Private {}

} // namespace Er {}