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

    void registerService(Er::Server::IServer* container);
    void unregisterService(Er::Server::IServer* container);

    Er::PropertyBag request(std::string_view request, std::string_view cookie, const Er::PropertyBag& args) override; 
    StreamId beginStream(std::string_view request, std::string_view cookie, const Er::PropertyBag& args) override;
    void endStream(StreamId id) override;
    Er::PropertyBag next(StreamId id) override;

private:
    Er::PropertyBag killProcess(const Er::PropertyBag& args); 
    Er::PropertyBag processProps(const Er::PropertyBag& args); 
    Er::PropertyBag processPropsExt(const Er::PropertyBag& args); 

    Er::Log::ILog* const m_log;
    ProcFs m_procFs;

};

} // namespace Erp::ProcessMgr {}