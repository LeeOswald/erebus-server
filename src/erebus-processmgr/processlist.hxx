#pragma once

#include <erebus-processmgr/processmgr.hxx>
#include <erebus-processmgr/procfs.hxx>

#include <chrono>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

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
    enum class StreamType
    {
        ProcessList
    };

    struct Stream
        : public boost::noncopyable
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
        explicit ProcessListStream(StreamId id, std::vector<uint64_t>&& pids) noexcept
            : Stream(StreamType::ProcessList, id)
            , pids(std::move(pids))
        {}

        std::vector<uint64_t> pids;
        size_t next = 0;
    };

    Er::PropertyBag processDetails(const Er::PropertyBag& args);
    Er::PropertyBag processDetails(uint64_t pid);
    Er::PropertyBag kernelDetails();

    void dropStaleStreams() noexcept;

    StreamId beginProcessStream(const Er::PropertyBag& args);
    Er::PropertyBag nextProcess(ProcessListStream* stream);

    const unsigned kStreamTimeoutSeconds = 60;

    Er::Log::ILog* m_log;
    Er::ProcFs::ProcFs m_procFs;
    std::shared_mutex m_mutex;
    StreamId m_nextStreamId = 0;
    std::unordered_map<StreamId, std::unique_ptr<Stream>> m_streams;
};

} // namespace Private {}

} // namespace Er {}