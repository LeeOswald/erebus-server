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
    
CoreService::CoreService(Er::Log::ILog* log)
    : m_log(log)
{
    ErLogDebug(m_log, ErLogInstance("CoreService"), "CoreService()");
}

void CoreService::registerService(Er::Server::IServiceContainer* container)
{
    container->registerService(Er::Protocol::GenericRequests::GetVersion, this);
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

CoreService::StreamId CoreService::beginStream(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId)
{
    throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported request %s", std::string(request).c_str()));
}

void CoreService::endStream(StreamId id, std::optional<SessionId> sessionId)
{
}

Er::PropertyBag CoreService::next(StreamId id, std::optional<SessionId> sessionId)
{
    return Er::PropertyBag();
}


} // namespace Private {}

} // namespace Er {}