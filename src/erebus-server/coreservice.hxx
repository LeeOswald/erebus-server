#pragma once

#include <erebus-srv/erebus-srv.hxx>


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
    explicit CoreService(Er::Log::ILog* log);

    void registerService(Er::Server::IServiceContainer* container);
    void unregisterService(Er::Server::IServiceContainer* container);

    SessionId allocateSession() override;
    void deleteSession(SessionId id)  override;
    Er::PropertyBag request(std::string_view request, const Er::PropertyBag& args, SessionId sessionId) override;
    StreamId beginStream(std::string_view request, const Er::PropertyBag& args, SessionId sessionId) override;
    void endStream(StreamId id, SessionId sessionId) override;
    Er::PropertyBag next(StreamId id, SessionId sessionId) override;

private:
    Er::PropertyBag getVersion(const Er::PropertyBag& args);

    Er::Log::ILog* m_log;
};


} // namespace Private {}

} // namespace Er {}