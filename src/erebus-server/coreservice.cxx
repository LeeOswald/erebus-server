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
}
    
CoreService::CoreService(Er::Log::ILog* log)
    : m_log(log)
{
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

Er::PropertyBag CoreService::request(std::string_view request, const Er::PropertyBag& args, SessionId sessionId)
{
    if (request == Er::Protocol::GenericRequests::GetVersion)
        return getVersion(args);
    else
        throwGenericError(Er::Util::format("Unsupported request %s", std::string(request).c_str()));
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

CoreService::StreamId CoreService::beginStream(std::string_view request, const Er::PropertyBag& args, SessionId sessionId)
{
    throwGenericError(Er::Util::format("Unsupported request %s", std::string(request).c_str()));
}

void CoreService::endStream(StreamId id, SessionId sessionId)
{
}

Er::PropertyBag CoreService::next(StreamId id, SessionId sessionId)
{
    return Er::PropertyBag();
}


} // namespace Private {}

} // namespace Er {}