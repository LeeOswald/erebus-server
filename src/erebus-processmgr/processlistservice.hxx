#pragma once

#include <erebus-processmgr/erebus-processmgr.hxx>
#include <erebus-srv/plugin.hxx>

#include "globalscollector.hxx"
#include "processlistcollector.hxx"
#include "procfs.hxx"


#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace Erp::ProcessMgr
{


class ProcessListService final
    : public Er::Server::IService
    , public Er::NonCopyable
{
public:
    ~ProcessListService();
    explicit ProcessListService(Er::Log::ILog* log);

    void registerService(Er::Server::IServiceContainer* container);
    void unregisterService(Er::Server::IServiceContainer* container);

    SessionId allocateSession() override;
    void deleteSession(SessionId id)  override;
    Er::PropertyBag request(std::string_view request, const Er::PropertyBag& args, SessionId sessionId) override; 
    StreamId beginStream(std::string_view request, const Er::PropertyBag& args, SessionId sessionId) override;
    void endStream(StreamId id, SessionId sessionId) override;
    Er::PropertyBag next(StreamId id, SessionId sessionId) override;

private:
    struct Stream;

    struct Session
        : public Er::NonCopyable
    {
        std::mutex mutex;
        std::chrono::steady_clock::time_point touched = std::chrono::steady_clock::now();
        SessionId id;
        std::unordered_map<StreamId, std::shared_ptr<Stream>> streams;
        StreamId nextStreamId = 1;

        std::unique_ptr<ProcessListCollector> collector;

        explicit Session(SessionId id) noexcept
            : id(id)
        {}
    };

    struct Stream
        : public Er::NonCopyable
    {
        enum class Type
        {
            ProcessListDiff
        };

        std::chrono::steady_clock::time_point touched = std::chrono::steady_clock::now();
        Type type;
        StreamId id;

        explicit Stream(Type type, StreamId id) noexcept
            : type(type)
            , id(id)
        {}
    };

    struct ProcessListDiffStream final
        : public Stream
    {
        explicit ProcessListDiffStream(StreamId id, Er::PropertyBag&& globals, ProcessListCollector::ProcessInfoCollectionDiff&& processes) noexcept
            : Stream(Type::ProcessListDiff, id)
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

        Er::PropertyBag globals;
        ProcessListCollector::ProcessInfoCollectionDiff processes;
        Stage stage = Stage::Globals;
        size_t next = 0;
    };

    static Er::ProcessMgr::ProcessProps::PropMask getProcessPropMask(const Er::PropertyBag& args);
    static Er::ProcessMgr::GlobalProps::PropMask getProcessesGlobalPropMask(const Er::PropertyBag& args);

    Er::PropertyBag processesGlobal(Session* session, Er::ProcessMgr::GlobalProps::PropMask required, std::optional<uint64_t> processCount);

    std::shared_ptr<Session> getSession(SessionId id);
    void dropStaleSessions() noexcept;

    void dropStaleStreams(Session* session) noexcept;

    StreamId beginProcessDiffStream(const Er::PropertyBag& args, Session* session);
    Er::PropertyBag nextProcessDiff(ProcessListDiffStream* stream, Session* session);

    const unsigned kSessionTimeoutSeconds = 60 * 60;
    const unsigned kStreamTimeoutSeconds = 60;

    Er::Log::ILog* const m_log;
    ProcFs m_procFs;
    GlobalsCollector m_globalsCollector;
   
    std::shared_mutex m_mutex;
    SessionId m_nextSessionId = 1;
    std::unordered_map<StreamId, std::shared_ptr<Session>> m_sessions;
};

} // namespace Erp::ProcessMgr {}