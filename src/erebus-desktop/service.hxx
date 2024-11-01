#pragma once

#include <erebus-desktop/erebus-desktop.hxx>
#include <erebus-srv/erebus-srv.hxx>


namespace Erp::Desktop
{


class IconResolver;
class IconCache;


class Service final
    : public Er::Server::IService
    , public Er::NonCopyable
{
public:
    ~Service();
    explicit Service(Er::Log::ILog* log, std::shared_ptr<Erp::Desktop::IconResolver> iconResolver, std::shared_ptr<IconCache> iconCache);

    void registerService(Er::Server::IServer* container);
    void unregisterService(Er::Server::IServer* container);

    Er::PropertyBag request(std::string_view request, std::string_view cookie, const Er::PropertyBag& args) override; 
    StreamId beginStream(std::string_view request, std::string_view cookie, const Er::PropertyBag& args) override;
    void endStream(StreamId id) override;
    Er::PropertyBag next(StreamId id) override;

private:
    Er::PropertyBag queryIcon(const Er::PropertyBag& args);
    Er::PropertyBag packIcon(std::shared_ptr<IconCache::IconData> icon);

    Er::Log::ILog* const m_log;
    std::shared_ptr<Erp::Desktop::IconResolver> m_iconResolver;
    std::shared_ptr<IconCache> m_iconCache;
};


} // namespace Erp::Desktop {}
