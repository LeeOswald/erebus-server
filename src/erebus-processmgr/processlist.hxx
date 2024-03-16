#pragma once

#include <erebus-processmgr/processmgr.hxx>
#include <erebus-processmgr/processprops.hxx>
#include <erebus-processmgr/procfs.hxx>

#include "processlistdiff.hxx"

#include <atomic>
#include <chrono>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace Er
{

namespace Private
{

class IconManager;

class ProcessList final
    : public Er::Server::IService
    , public Er::NonCopyable
{
public:
    ~ProcessList();
    explicit ProcessList(Er::Log::ILog* log, IconManager* iconManager);

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
        explicit ProcessListStream(StreamId id, Er::ProcessProps::PropMask required, std::vector<uint64_t>&& pids) noexcept
            : Stream(StreamType::ProcessList, id)
            , required(required)
            , pids(std::move(pids))
        {}

        Er::ProcessProps::PropMask required;
        std::vector<uint64_t> pids;
        size_t next = 0;
        double utime = 0.0;
        double stime = 0.0;
    };

    struct ProcessListDiffStream final
        : public Stream
    {
        explicit ProcessListDiffStream(StreamId id, Er::ProcessProps::PropMask required, ProcessCollectionDiff&& diff) noexcept
            : Stream(StreamType::ProcessListDiff, id)
            , diff(std::move(diff))
        {}

        enum class Stage
        {
            Removed,
            Modified,
            Added
        };

        ProcessCollectionDiff diff;
        Stage stage = Stage::Removed;
        size_t next = 0;
    };

    static Er::ProcessProps::PropMask getProcessPropMask(const Er::PropertyBag& args);
    Er::PropertyBag processDetails(const Er::PropertyBag& args, Er::ProcessProps::PropMask required);

    static std::pair<Er::ProcessesGlobal::PropMask, bool> getProcessesGlobalPropMask(const Er::PropertyBag& args); // mask:lazy
    Er::PropertyBag processesGlobal(const Er::PropertyBag& args, Er::ProcessesGlobal::PropMask required, bool lazy);

    Session* getSession(std::optional<SessionId> id);
    void dropStaleSessions() noexcept;

    void dropStaleStreams() noexcept;

    StreamId beginProcessStream(const Er::PropertyBag& args);
    Er::PropertyBag nextProcess(ProcessListStream* stream);

    StreamId beginProcessDiffStream(const Er::PropertyBag& args, Session* session);
    Er::PropertyBag nextProcessDiff(ProcessListDiffStream* stream, Session* session);

    const unsigned kSessionTimeoutSeconds = 60 * 60;
    const unsigned kStreamTimeoutSeconds = 60;

    Er::Log::ILog* m_log;
    IconManager* m_iconManager;
    Er::ProcFs::ProcFs m_procFs;
    std::atomic<std::size_t> m_processCount = 0;
    std::atomic<double> m_stime = 0.0;
    std::atomic<double> m_utime = 0.0;
    std::shared_mutex m_mutex;
    SessionId m_nextSessionId = 0;
    StreamId m_nextStreamId = 0;
    std::unordered_map<StreamId, std::unique_ptr<Session>> m_sessions;
    std::unordered_map<StreamId, std::unique_ptr<Stream>> m_streams;
    
};

} // namespace Private {}

} // namespace Er {}