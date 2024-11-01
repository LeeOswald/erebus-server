#pragma once


#include <erebus-processmgr/erebus-processmgr.hxx>
#include <erebus-srv/cookies.hxx>
#include <erebus-srv/plugin.hxx>

#include "globalscollector.hxx"
#include "processlistcollector.hxx"
#include "procfs.hxx"


namespace Erp::ProcessMgr
{


class ProcessListService final
    : public Er::Server::IService
    , public Er::NonCopyable
{
public:
    ~ProcessListService();
    explicit ProcessListService(Er::Log::ILog* log);

    void registerService(Er::Server::IServer* container);
    void unregisterService(Er::Server::IServer* container);

    Er::PropertyBag request(std::string_view request, std::string_view cookie, const Er::PropertyBag& args) override; 
    [[nodiscard]] StreamId beginStream(std::string_view request, std::string_view cookie, const Er::PropertyBag& args) override;
    void endStream(StreamId id) override;
    Er::PropertyBag next(StreamId id) override;

private:
    struct Session
        : public Er::NonCopyable
    {
        Session() noexcept = default;

        std::unique_ptr<ProcessListCollector> collector;
    };

    using Sessions = Er::Server::Cookies<std::string, Session>;
    using SessionRef = Sessions::Ref;

    struct Stream
        : public Er::NonCopyable
    {
        enum class Type
        {
            ProcessListDiff
        };

        const Type type;

        virtual ~Stream() = default;

        constexpr Stream(Type type) noexcept
            : type(type)
        {}
    };
    
    struct ProcessListDiffStream final
        : public Stream
    {
        explicit ProcessListDiffStream(SessionRef&& session, Er::PropertyBag&& globals, ProcessListCollector::ProcessInfoCollectionDiff&& processes) noexcept
            : Stream(Type::ProcessListDiff)
            , session(std::move(session))
            , globals(std::move(globals))
            , processes(std::move(processes))
        {}

        enum class Stage
        {
            Globals,
            Removed,
            Modified,
            Added
        };

        SessionRef session;
        Er::PropertyBag globals;
        ProcessListCollector::ProcessInfoCollectionDiff processes;
        Stage stage = Stage::Globals;
        size_t next = 0;
    };

    static Er::ProcessMgr::ProcessProps::PropMask getProcessPropMask(const Er::PropertyBag& args);
    static Er::ProcessMgr::GlobalProps::PropMask getProcessesGlobalPropMask(const Er::PropertyBag& args);

    Er::PropertyBag processesGlobal(Er::ProcessMgr::GlobalProps::PropMask required, std::optional<uint64_t> processCount);

    [[nodiscard]] StreamId beginProcessDiffStream(SessionRef&& session, const Er::PropertyBag& args);
    Er::PropertyBag nextProcessDiff(ProcessListDiffStream* stream);

    Er::Log::ILog* const m_log;
    ProcFs m_procFs;
    GlobalsCollector m_globalsCollector;
    Sessions m_sessions;
};

} // namespace Erp::ProcessMgr {}