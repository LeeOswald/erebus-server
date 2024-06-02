#include "service.hxx"

#include <erebus/exception.hxx>
#include <erebus/util/format.hxx>

namespace Er
{

namespace Desktop
{

namespace Private
{


Service::~Service()
{
    ErLogDebug(m_log, ErLogInstance("Er::Desktop::Private::Service"), "~Service()");
}

Service::Service(Er::Log::ILog* log)
    : m_log(log)
{
    ErLogDebug(m_log, ErLogInstance("Er::Desktop::Private::Service"), "Service()");
}

void Service::registerService(Er::Server::IServiceContainer* container)
{
    container->registerService(Er::Desktop::Requests::QueryIcon, this);
    container->registerService(Er::Desktop::Requests::PullIcons, this);
}

void Service::unregisterService(Er::Server::IServiceContainer* container)
{
    container->unregisterService(this);
}

Service::SessionId Service::allocateSession()
{
    std::unique_lock l(m_mutexSession);
    auto id = m_nextSessionId++;

    m_sessions.insert({ id, std::make_unique<Session>(id) });

    ErLogDebug(m_log, ErLogInstance("Er::Desktop::Private::Service"), "Started session %d", id);

    return id;
}

void Service::deleteSession(SessionId id)
{
    std::unique_lock l(m_mutexSession);

    auto it = m_sessions.find(id);
    if (it == m_sessions.end())
        throw Er::Exception(ER_HERE(), Er::Util::format("Non-existent session %d", id));

    m_sessions.erase(it);

    dropStaleSessions();

    ErLogDebug(m_log, ErLogInstance("Er::Desktop::Private::Service"), "Ended session %d", id);    
}

Service::Session* Service::getSession(std::optional<SessionId> id)
{
    if (!id)
        throw Er::Exception(ER_HERE(), "Session not specified");

    std::unique_lock l(m_mutexSession);

    auto it = m_sessions.find(*id);
    if (it == m_sessions.end())
        throw Er::Exception(ER_HERE(), "Session not found");

    it->second->touched = std::chrono::steady_clock::now();

    return it->second.get();
}

void Service::dropStaleStreams() noexcept
{
    auto now = std::chrono::steady_clock::now();

    for (auto it = m_streams.begin(); it != m_streams.end();)
    {
        auto d = std::chrono::duration_cast<std::chrono::seconds>(now - it->second->touched);
        if (d.count() > kStreamTimeoutSeconds)
        {
            auto next = std::next(it);
            ErLogWarning(m_log, ErLogInstance("Er::Desktop::Private::Service"), "Dropping stale stream %d", it->first);
            m_streams.erase(it);
            it = next;
        }
        else
        {
            ++it;
        }
    }
}

void Service::dropStaleSessions() noexcept
{
    auto now = std::chrono::steady_clock::now();

    for (auto it = m_sessions.begin(); it != m_sessions.end();)
    {
        auto d = std::chrono::duration_cast<std::chrono::seconds>(now - it->second->touched);
        if (d.count() > kSessionTimeoutSeconds)
        {
            auto next = std::next(it);
            ErLogWarning(m_log, ErLogInstance("Er::Desktop::Private::Service"), "Dropping stale session %d", it->first);
            m_sessions.erase(it);
            it = next;
        }
        else
        {
            ++it;
        }
    }
}

Er::PropertyBag Service::request(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId)
{
    if (request == Er::Desktop::Requests::QueryIcon)
    {
        return queryIcon(args, sessionId);
    }

    throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported request %s", std::string(request).c_str()));
}

Er::PropertyBag Service::queryIcon(const Er::PropertyBag& args, std::optional<SessionId> sessionId)
{
    return Er::PropertyBag();
}

} // namespace Private {}

} // namespace Desktop {}

} // namespace Er {}