#pragma once

#include <erebus-srv/erebus-srv.hxx>

#include <atomic>
#include <mutex>
#include <random>
#include <unordered_map>

namespace Erp::Server
{

class CoreService
    : public Er::Server::IService
    , public std::enable_shared_from_this<CoreService>
{
public:
    ~CoreService();

    static auto create(Er::Log::ILog* log)
    {
        return std::shared_ptr<CoreService>(new CoreService(log));
    }

    void registerService(Er::Server::IServer* container) override;
    void unregisterService(Er::Server::IServer* container) override;

    Er::PropertyBag request(std::string_view request, std::string_view cookie, const Er::PropertyBag& args) override;
    StreamId beginStream(std::string_view request, std::string_view cookie, const Er::PropertyBag& args) override;
    void endStream(StreamId id) override;
    Er::PropertyBag next(StreamId id) override;

private:
    CoreService(Er::Log::ILog* log);

    enum class StreamType
    {
        Ping
    };

    struct Stream
    {
        using Ptr = std::shared_ptr<Stream>;

        StreamType type;

        constexpr Stream(StreamType type) noexcept
            : type(type)
        {
        }

        virtual ~Stream() = default;
    };

    struct PingStream
        : public Stream
    {
        std::atomic<uint32_t> seqId = 0;
        uint32_t dataSize;
        std::optional<uint32_t> sender;

        PingStream(uint32_t dataSize, uint32_t const* sender) noexcept
            : Stream(StreamType::Ping)
            , dataSize(dataSize)
        {
            if (sender)
                this->sender = *sender;
        }
    };

    Er::PropertyBag getVersion(const Er::PropertyBag& args);
    Er::PropertyBag ping(const Er::PropertyBag& args);
    StreamId beginPingStream(const Er::PropertyBag& args);
    Er::PropertyBag nextPing(StreamId id, PingStream* stream);

    auto seed()
    {
        std::lock_guard l(m_mutexRd);
        return m_rd();
    }

    Er::Log::ILog* m_log;
    
    std::mutex m_mutexRd;
    std::random_device m_rd;
    
    std::mutex m_mutexStreams;
    StreamId m_nextStreamId = 0;
    std::unordered_map<StreamId, Stream::Ptr> m_streams;
};


} // namespace Erp::Server {}