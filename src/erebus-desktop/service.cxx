#include "iconcache.hxx"
#include "iconresolver.hxx"
#include "service.hxx"

#include <erebus-desktop/protocol.hxx>

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

Service::Service(Er::Log::ILog* log, std::shared_ptr<Er::Desktop::Private::IconResolver> iconResolver, std::shared_ptr<IconCache> iconCache)
    : m_log(log)
    , m_iconResolver(iconResolver)
    , m_iconCache(iconCache)
{
    ErLogDebug(m_log, ErLogInstance("Er::Desktop::Private::Service"), "Service()");
}

void Service::registerService(Er::Server::IServiceContainer* container)
{
    container->registerService(Er::Desktop::Requests::QueryIcon, this);
}

void Service::unregisterService(Er::Server::IServiceContainer* container)
{
    container->unregisterService(this);
}

Service::SessionId Service::allocateSession()
{
    return 0;
}

void Service::deleteSession(SessionId id)
{
}

Service::StreamId Service::beginStream(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId)
{
    throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported request %s", std::string(request).c_str()));
}

void Service::endStream(StreamId id, std::optional<SessionId> sessionId)
{
}

Er::PropertyBag Service::next(StreamId id, std::optional<SessionId> sessionId)
{
    return Er::PropertyBag();
}

Er::PropertyBag Service::request(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId)
{
    if (request == Er::Desktop::Requests::QueryIcon)
    {
        return queryIcon(args);
    }

    throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported request %s", std::string(request).c_str()));
}

Er::PropertyBag Service::queryIcon(const Er::PropertyBag& args)
{
    auto iconSize = Er::getProperty<Er::Desktop::Props::IconSize>(args);
    if (!iconSize)
        throw Er::Exception(ER_HERE(), "Icon size not specified");

    if ((*iconSize != uint32_t(IconSize::Large)) && (*iconSize != uint32_t(IconSize::Small)))
        throw Er::Exception(ER_HERE(), "Unsupported icon size");

    auto iconName = Er::getProperty<Er::Desktop::Props::IconName>(args);
    if (iconName)
    {
        auto iconData = m_iconCache->lookupByName(*iconName, IconSize(*iconSize));
        return packIcon(iconData);
    }

    auto pid = Er::getProperty<Er::Desktop::Props::Pid>(args);
    if (pid)
    {
        auto name = m_iconResolver->lookupIcon(*pid);
        if (!name.empty())
        {
            auto iconData = m_iconCache->lookupByName(name, IconSize(*iconSize));
            return packIcon(iconData);
        }
    }

    return Er::PropertyBag();
}

Er::PropertyBag Service::packIcon(std::shared_ptr<IconCache::IconData> icon)
{
    Er::PropertyBag response;

    Er::addProperty<Er::Desktop::Props::IconState>(response, static_cast<uint32_t>(icon->state));
    
    if (!icon->raw.empty())
        Er::addProperty<Er::Desktop::Props::Icon>(response, icon->raw);

    return response;
}

} // namespace Private {}

} // namespace Desktop {}

} // namespace Er {}