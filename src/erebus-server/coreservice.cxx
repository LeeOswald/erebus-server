#include "coreservice.hxx"

#include <erebus/exception.hxx>
#include <erebus/protocol.hxx>
#include <erebus-srv/global_requests.hxx>


#if ER_POSIX
    #include <sys/utsname.h>
#endif

#include "erebus-version.h"

#include <erebus/trace.hxx>

namespace Erp::Server
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
    container->registerService(Er::Server::Requests::GetVersion, this);
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
    TraceMethod("CoreService");
    if (request == Er::Server::Requests::GetVersion)
        return getVersion(args);
    else
        ErThrow(Er::format("Unsupported request {}", request));
}

Er::PropertyBag CoreService::getVersion(const Er::PropertyBag& args)
{
    TraceMethod("CoreService");
    Er::PropertyBag reply;

#if ER_WINDOWS
    std::string platform("Windows");
#elif ER_LINUX
    std::string platform;
    struct utsname u = {};
    if (::uname(&u) == 0)
    {
        platform = Er::format("{} {}", u.sysname, u.release);
    }
#endif

    auto version = Er::format("{} {}.{}.{} {}", ER_APPLICATION_NAME, ER_VERSION_MAJOR, ER_VERSION_MINOR, ER_VERSION_PATCH, ER_COPYRIGHT);

    Er::addProperty<Er::Server::Props::SystemName>(reply, std::move(platform));
    Er::addProperty<Er::Server::Props::ServerVersion>(reply, std::move(version));

    return reply;
}

CoreService::StreamId CoreService::beginStream(std::string_view request, const Er::PropertyBag& args, SessionId sessionId)
{
    ErThrow(Er::format("Unsupported request {}", request));
}

void CoreService::endStream(StreamId id, SessionId sessionId)
{
}

Er::PropertyBag CoreService::next(StreamId id, SessionId sessionId)
{
    return Er::PropertyBag();
}


} // namespace Erp::Server {}