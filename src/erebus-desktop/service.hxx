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

    void registerService(Er::Server::IServiceContainer* container);
    void unregisterService(Er::Server::IServiceContainer* container);

    SessionId allocateSession() override;
    void deleteSession(SessionId id)  override;
    Er::PropertyBag request(std::string_view request, const Er::PropertyBag& args, SessionId sessionId) override; 
    StreamId beginStream(std::string_view request, const Er::PropertyBag& args, SessionId sessionId) override;
    void endStream(StreamId id, SessionId sessionId) override;
    Er::PropertyBag next(StreamId id, SessionId sessionId) override;

private:
    Er::PropertyBag queryIcon(const Er::PropertyBag& args);
    Er::PropertyBag packIcon(std::shared_ptr<IconCache::IconData> icon);

    Er::Log::ILog* const m_log;
    std::shared_ptr<Erp::Desktop::IconResolver> m_iconResolver;
    std::shared_ptr<IconCache> m_iconCache;
};


} // namespace Erp::Desktop {}
