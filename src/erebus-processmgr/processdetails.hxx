#pragma once

#include <erebus-processmgr/processmgr.hxx>
#include <erebus-processmgr/processprops.hxx>


namespace Er
{

namespace Private
{


class ProcessDetails final
    : public Er::Server::IService
    , public Er::NonCopyable
{
public:
    ~ProcessDetails();
    explicit ProcessDetails(Er::Log::ILog* log);

    void registerService(Er::Server::IServiceContainer* container);
    void unregisterService(Er::Server::IServiceContainer* container);

    SessionId allocateSession() override;
    void deleteSession(SessionId id)  override;
    Er::PropertyBag request(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId) override; 
    StreamId beginStream(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId) override;
    void endStream(StreamId id, std::optional<SessionId> sessionId) override;
    Er::PropertyBag next(StreamId id, std::optional<SessionId> sessionId) override;

private:
    Er::PropertyBag killProcess(const Er::PropertyBag& args); 

    Er::Log::ILog* const m_log;

};

} // namespace Private {}

} // namespace Er {}