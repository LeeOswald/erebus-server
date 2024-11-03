#include "iconcache.hxx"
#include "iconresolver.hxx"
#include "service.hxx"

#include <erebus-desktop/erebus-desktop.hxx>

#include <erebus/exception.hxx>


namespace Erp::Desktop
{


Service::~Service()
{
}

Service::Service(Er::Log::ILog* log, std::shared_ptr<Erp::Desktop::IconResolver> iconResolver, std::shared_ptr<IconCache> iconCache)
    : m_log(log)
    , m_iconResolver(iconResolver)
    , m_iconCache(iconCache)
{
}

void Service::registerService(Er::Server::IServer* container)
{
    container->registerService(Er::Desktop::Requests::QueryIcon, shared_from_this());
}

void Service::unregisterService(Er::Server::IServer* container)
{
    container->unregisterService(this);
}

Service::StreamId Service::beginStream(std::string_view request, std::string_view cookie, const Er::PropertyBag& args)
{
    ErThrow(Er::format("Unsupported request {}", request));
}

void Service::endStream(StreamId id)
{
}

Er::PropertyBag Service::next(StreamId id)
{
    return Er::PropertyBag();
}

Er::PropertyBag Service::request(std::string_view request, std::string_view cookie, const Er::PropertyBag& args)
{
    if (request == Er::Desktop::Requests::QueryIcon)
    {
        return queryIcon(args);
    }

    ErThrow(Er::format("Unsupported request {}", request));
}

Er::PropertyBag Service::queryIcon(const Er::PropertyBag& args)
{
    auto iconSize = Er::getPropertyValue<Er::Desktop::Props::IconSize>(args);
    if (!iconSize)
        ErThrow("Icon size not specified");

    if ((*iconSize != uint32_t(IconSize::Large)) && (*iconSize != uint32_t(IconSize::Small)))
        ErThrow("Unsupported icon size");

    auto iconName = Er::getPropertyValue<Er::Desktop::Props::IconName>(args);
    if (iconName)
    {
        auto iconData = m_iconCache->lookupByName(*iconName, IconSize(*iconSize));
        return packIcon(iconData);
    }

    auto pid = Er::getPropertyValue<Er::Desktop::Props::Pid>(args);
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


} // namespace Erp::Desktop {}