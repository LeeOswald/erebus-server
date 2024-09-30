#pragma once

#include <erebus/propertybag.hxx>
#include <erebus-processmgr/erebus-processmgr.hxx>
#include <erebus-srv/plugin.hxx>

#include "procfs.hxx"

namespace Erp::ProcessMgr
{


class ProcessDetailsService final
    : public Er::Server::IService
    , public Er::NonCopyable
{
public:
    ~ProcessDetailsService();
    explicit ProcessDetailsService(Er::Log::ILog* log);

    void registerService(Er::Server::IServiceContainer* container);
    void unregisterService(Er::Server::IServiceContainer* container);

    SessionId allocateSession() override;
    void deleteSession(SessionId id)  override;
    Er::PropertyBag request(std::string_view request, const Er::PropertyBag& args, SessionId sessionId) override; 
    StreamId beginStream(std::string_view request, const Er::PropertyBag& args, SessionId sessionId) override;
    void endStream(StreamId id, SessionId sessionId) override;
    Er::PropertyBag next(StreamId id, SessionId sessionId) override;

private:
    Er::PropertyBag killProcess(const Er::PropertyBag& args); 
    Er::PropertyBag processProps(const Er::PropertyBag& args); 
    Er::PropertyBag processPropsExt(const Er::PropertyBag& args); 

    Er::Log::ILog* const m_log;
    ProcFs m_procFs;

};

} // namespace Erp::ProcessMgr {}