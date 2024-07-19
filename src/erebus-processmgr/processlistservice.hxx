#pragma once

#include <erebus-processmgr/erebus-processmgr.hxx>
#include <erebus-srv/plugin.hxx>

#include "processlistdiff.hxx"
#include "procfs.hxx"


#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace Erp
{

namespace ProcessMgr
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
    Er::PropertyBag request(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId) override; 
    StreamId beginStream(std::string_view request, const Er::PropertyBag& args, std::optional<SessionId> sessionId) override;
    void endStream(StreamId id, std::optional<SessionId> sessionId) override;
    Er::PropertyBag next(StreamId id, std::optional<SessionId> sessionId) override;

private:
    struct Session
        : public Er::NonCopyable
    {
        std::chrono::steady_clock::time_point touched = std::chrono::steady_clock::now();
        SessionId id;
        ProcessCollection processes;

        explicit Session(SessionId id) noexcept
            : id(id)
        {}
    };

    enum class StreamType
    {
        ProcessList,
        ProcessListDiff
    };

    struct Stream
        : public Er::NonCopyable
    {
        std::chrono::steady_clock::time_point touched = std::chrono::steady_clock::now();
        StreamType type;
        StreamId id;

        explicit Stream(StreamType type, StreamId id) noexcept
            : type(type)
            , id(id)
        {}
    };

    struct ProcessListStream final
        : public Stream
    {
        explicit ProcessListStream(StreamId id, Er::ProcessMgr::ProcessProps::PropMask required, std::vector<uint64_t>&& pids) noexcept
            : Stream(StreamType::ProcessList, id)
            , required(required)
            , pids(std::move(pids))
        {}

        Er::ProcessMgr::ProcessProps::PropMask required;
        std::vector<uint64_t> pids;
        size_t next = 0;
    };

    struct ProcessListDiffStream final
        : public Stream
    {
        explicit ProcessListDiffStream(StreamId id, Er::ProcessMgr::ProcessProps::PropMask required, ProcessCollectionDiff&& diff) noexcept
            : Stream(StreamType::ProcessListDiff, id)
            , diff(std::move(diff))
        {}

        enum class Stage
        {
            Globals,
            Removed,
            Modified,
            Added
        };

        ProcessCollectionDiff diff;
        Stage stage = Stage::Globals;
        size_t next = 0;
    };

    static Er::ProcessMgr::ProcessProps::PropMask getProcessPropMask(const Er::PropertyBag& args);
    Er::PropertyBag processDetails(const Er::PropertyBag& args, Er::ProcessMgr::ProcessProps::PropMask required);

    static Er::ProcessMgr::ProcessesGlobal::PropMask getProcessesGlobalPropMask(const Er::PropertyBag& args);
    Er::PropertyBag processesGlobal(Er::ProcessMgr::ProcessesGlobal::PropMask required, std::optional<uint64_t> processCount);

    Session* getSession(std::optional<SessionId> id);
    void dropStaleSessions() noexcept;

    void dropStaleStreams() noexcept;

    StreamId beginProcessStream(const Er::PropertyBag& args);
    Er::PropertyBag nextProcess(ProcessListStream* stream);

    StreamId beginProcessDiffStream(const Er::PropertyBag& args, Session* session);
    Er::PropertyBag nextProcessDiff(ProcessListDiffStream* stream, Session* session);

    const unsigned kSessionTimeoutSeconds = 60 * 60;
    const unsigned kStreamTimeoutSeconds = 60;

    Er::Log::ILog* const m_log;
    ProcFs m_procFs;
   
    std::shared_mutex m_mutexSession;
    SessionId m_nextSessionId = 0;
    StreamId m_nextStreamId = 0;
    std::unordered_map<StreamId, std::unique_ptr<Session>> m_sessions;
    std::unordered_map<StreamId, std::unique_ptr<Stream>> m_streams;
};

} // namespace ProcessMgr {}

} // namespace Erp {}