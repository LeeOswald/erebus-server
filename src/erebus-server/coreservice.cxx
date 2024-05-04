#include "coreservice.hxx"

#include <erebus/exception.hxx>
#include <erebus/protocol.hxx>
#include <erebus/util/format.hxx>

#if ER_POSIX
    #include <sys/utsname.h>
#endif

#include "erebus-version.h"

namespace Er
{

namespace Private
{

CoreService::~CoreService()
{
    ErLogDebug(m_log, ErLogInstance("CoreService"), "~CoreService()");
}
    
CoreService::CoreService(Er::Log::ILog* log, Er::Server::Private::IUserDb* userDb)
    : m_log(log)
    , m_userDb(userDb)
{
    ErLogDebug(m_log, ErLogInstance("CoreService"), "CoreService()");
}

void CoreService::registerService(Er::Server::IServiceContainer* container)
{
    container->registerService(Er::Protocol::GenericRequests::GetVersion, this);
    container->registerService(Er::Protocol::GenericRequests::AddUser, this);
    container->registerService(Er::Protocol::GenericRequests::RemoveUser, this);
    container->registerService(Er::Protocol::GenericRequests::ListUsers, this);
}

void CoreService::unregisterService(Er::Server::IServiceContainer* container)
{
    container->unregisterService(this);
}

CoreService::SessionId CoreService::allocateSession()
{
    return SessionId(0);
}

void CoreService::deleteSession(SessionId id)
{
}

Er::PropertyBag CoreService::request(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId)
{
    if (request == Er::Protocol::GenericRequests::GetVersion)
        return getVersion(args);
    else if (request == Er::Protocol::GenericRequests::AddUser)
        return addUser(args);
    else if (request == Er::Protocol::GenericRequests::RemoveUser)
        return removeUser(args);
    else
        throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported request %s", std::string(request).c_str()));
}

Er::PropertyBag CoreService::getVersion(const Er::PropertyBag& args)
{
    Er::PropertyBag reply;

#if ER_WINDOWS
    std::string platform("Windows");
#elif ER_LINUX
    std::string platform;
    struct utsname u = {};
    if (::uname(&u) == 0)
    {
        platform = Er::Util::format("%s %s", u.sysname, u.release);
    }
#endif

    auto version = Er::Util::format("%s %d.%d.%d %s", ER_APPLICATION_NAME, ER_VERSION_MAJOR, ER_VERSION_MINOR, ER_VERSION_PATCH, ER_COPYRIGHT);

    Er::addProperty<Er::Protocol::Props::RemoteSystemDesc>(reply, std::move(platform));
    Er::addProperty<Er::Protocol::Props::ServerVersionString>(reply, std::move(version));

    return reply;
}

Er::PropertyBag CoreService::addUser(const Er::PropertyBag& args)
{
    Er::PropertyBag reply;

    auto user = Er::getProperty<Er::Protocol::Props::User>(args);
    if (!user)
        throw Er::Exception(ER_HERE(), "User name expected");

    auto salt = Er::getProperty<Er::Protocol::Props::Salt>(args);
    if (!salt)
        throw Er::Exception(ER_HERE(), "Salt expected");

    auto password = Er::getProperty<Er::Protocol::Props::Password>(args);
    if (!password)
        throw Er::Exception(ER_HERE(), "Password expected");

    m_userDb->add(Er::Server::Private::User(*user, *salt, *password));
    m_userDb->save();

    return reply;
}

Er::PropertyBag CoreService::removeUser(const Er::PropertyBag& args)
{
    Er::PropertyBag reply;

    auto user = Er::getProperty<Er::Protocol::Props::User>(args);
    if (!user)
        throw Er::Exception(ER_HERE(), "User name expected");

    m_userDb->remove(*user);
    m_userDb->save();

    return reply;
}

CoreService::StreamId CoreService::beginStream(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId)
{
    if (request == Er::Protocol::GenericRequests::ListUsers)
        return beginUserListStream();
    else
        throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported request %s", std::string(request).c_str()));
}

void CoreService::endStream(StreamId id, std::optional<SessionId> sessionId)
{
    std::unique_lock l(m_mutex);

    auto it = m_streams.find(id);
    if (it == m_streams.end())
        throw Er::Exception(ER_HERE(), Er::Util::format("Non-existent stream %d", id));

    m_streams.erase(it);

    dropStaleStreams();
}

Er::PropertyBag CoreService::next(StreamId id, std::optional<SessionId> sessionId)
{
    Stream* stream = nullptr;
    {
        std::unique_lock l(m_mutex);
        auto it = m_streams.find(id);
        if (it == m_streams.end())
            throw Er::Exception(ER_HERE(), Er::Util::format("Non-existent stream %d", id));

        stream = it->second.get();

        stream->touched = std::chrono::steady_clock::now();
    }

    if (stream->type == StreamType::UserList)
        return nextUser(static_cast<UserListStream*>(stream));

    ErAssert(!"Unknown stream type");
    return Er::PropertyBag();
}

CoreService::StreamId CoreService::beginUserListStream()
{
    auto users = m_userDb->enumerate();

    auto streamId = m_nextStreamId++;
    auto stream = std::make_unique<UserListStream>(streamId, std::move(users));
    m_streams.insert({ streamId, std::move(stream) });

    return streamId;
}

Er::PropertyBag CoreService::nextUser(UserListStream* stream)
{
    Er::PropertyBag userInfo;

    if (stream->next >= stream->users.size())
    {
        return userInfo; // end of stream
    }
        
    Er::addProperty<Er::Protocol::Props::User>(userInfo, std::move(stream->users[stream->next].name));
    ++stream->next;

    return userInfo;
}

void CoreService::dropStaleStreams() noexcept
{
    auto now = std::chrono::steady_clock::now();

    for (auto it = m_streams.begin(); it != m_streams.end();)
    {
        auto d = std::chrono::duration_cast<std::chrono::seconds>(now - it->second->touched);
        if (d.count() > kStreamTimeoutSeconds)
        {
            auto next = std::next(it);
            ErLogWarning(m_log, ErLogInstance("CoreService"), "Dropping stale stream %d", it->first);
            m_streams.erase(it);
            it = next;
        }
        else
        {
            ++it;
        }
    }
}

} // namespace Private {}

} // namespace Er {}