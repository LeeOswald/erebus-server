#pragma once

#include <erebus-processmgr/processmgr.hxx>
#include <erebus-processmgr/procfs.hxx>


namespace Er
{

namespace Private
{

class ProcessList final
    : public Er::Server::IService
    , public boost::noncopyable
{
public:
    ~ProcessList();
    explicit ProcessList(Er::Log::ILog* log);

    Er::PropertyBag request(const std::string& request, const Er::PropertyBag& args) override; 
    StreamId beginStream(const std::string& request, const Er::PropertyBag& args) override;
    void endStream(StreamId id) override;
    Er::PropertyBag next(StreamId id) override;

private:
    Er::PropertyBag processDetails(const Er::PropertyBag& args);
    Er::PropertyBag processDetails(uint64_t pid);

    Er::Log::ILog* m_log;
    Er::ProcFs::ProcFs m_procFs;
};

} // namespace Private {}

} // namespace Er {}