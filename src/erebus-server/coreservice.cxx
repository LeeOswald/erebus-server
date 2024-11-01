#include "coreservice.hxx"

#include <erebus/exception.hxx>
#include <erebus/protocol.hxx>
#include <erebus-srv/global_requests.hxx>


#if ER_POSIX
    #include <sys/utsname.h>
#endif


#include <sstream>

#include "erebus-version.h"


namespace Erp::Server
{

namespace
{

Er::Binary generateData(std::random_device::result_type seed, uint32_t size)
{
    static const char ValidChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
    static const size_t ValidCharsCount = sizeof(ValidChars) - 1;

    std::mt19937 gen(seed);
    std::uniform_int_distribution<> charDistrib(0, ValidCharsCount - 1);

    std::ostringstream ss;
    while (size--)
    {
        auto ch = ValidChars[charDistrib(gen)];
        ss << ch;
    }

    return Er::Binary(ss.str());
}


} // namespace {}

CoreService::~CoreService()
{
}
    
CoreService::CoreService(Er::Log::ILog* log)
    : m_log(log)
{
}

void CoreService::registerService(Er::Server::IServer* container)
{
    container->registerService(Er::Server::Requests::GetVersion, this);
    container->registerService(Er::Server::Requests::Ping, this);
}

void CoreService::unregisterService(Er::Server::IServer* container)
{
    container->unregisterService(this);
}

Er::PropertyBag CoreService::request(std::string_view request, std::string_view cookie, const Er::PropertyBag& args)
{
    Er::Log::debug(m_log, "CoreService::request");
    Er::Log::Indent idt(m_log);

    if (request == Er::Server::Requests::GetVersion)
        return getVersion(args);
    else if (request == Er::Server::Requests::Ping)
        return ping(args);
    else
        ErThrow(Er::format("Unsupported request {}", request));
}

Er::PropertyBag CoreService::getVersion(const Er::PropertyBag& args)
{
    Er::Log::debug(m_log, "CoreService::request");
    Er::Log::Indent idt(m_log);

    Er::PropertyBag reply;

#if ER_WINDOWS
    std::string platform("Windows");
#elif ER_LINUX
    std::string platform;
    struct utsname u = {};
    if (::uname(&u) == 0)
    {
        platform = Er::format("{} {}", u.sysname, u.release);
    }
#endif

    auto version = Er::format("{} {}.{}.{} {}", ER_APPLICATION_NAME, ER_VERSION_MAJOR, ER_VERSION_MINOR, ER_VERSION_PATCH, ER_COPYRIGHT);

    Er::addProperty<Er::Server::Props::SystemName>(reply, std::move(platform));
    Er::addProperty<Er::Server::Props::ServerVersion>(reply, std::move(version));

    return reply;
}

Er::PropertyBag CoreService::ping(const Er::PropertyBag& args)
{
    Er::Log::debug(m_log, "CoreService::ping");
    Er::Log::Indent idt(m_log);

    Er::PropertyBag reply;

    auto sender = Er::getPropertyValue<Er::Server::Props::PingSender>(args);
    auto dataSize = Er::getPropertyValueOr<Er::Server::Props::PingDataSize>(args, 4096);
    auto data = generateData(seed(), dataSize);

    if (sender)
        Er::addProperty<Er::Server::Props::PingSender>(reply, *sender);

    Er::addProperty<Er::Server::Props::PingDataSize>(reply, dataSize);
    Er::addProperty<Er::Server::Props::PingData>(reply, std::move(data));

    return reply;
}

CoreService::StreamId CoreService::beginStream(std::string_view request, std::string_view cookie, const Er::PropertyBag& args)
{
    Er::Log::debug(m_log, "CoreService::beginStream");
    Er::Log::Indent idt(m_log);

    if (request == Er::Server::Requests::Ping)
        return beginPingStream(args);

    ErThrow(Er::format("Unsupported request {}", request));
}

void CoreService::endStream(StreamId id)
{
    Er::Log::debug(m_log, "CoreService::endStream");
    Er::Log::Indent idt(m_log);

    {
        std::lock_guard l(m_mutexStreams);

        auto it = m_streams.find(id);
        if (it == m_streams.end()) [[unlikely]]
        {
            Er::Log::error(m_log, "Non-existent stream id {}", id);
            return;
        }

        m_streams.erase(it);
    }

    Er::Log::debug(m_log, "Ended stream {}", id);
}

Er::PropertyBag CoreService::next(StreamId id)
{
    Er::Log::debug(m_log, "CoreService::next");
    Er::Log::Indent idt(m_log);

    Stream::Ptr stream;
    
    {
        std::lock_guard l(m_mutexStreams);

        auto it = m_streams.find(id);
        if (it != m_streams.end()) [[likely]]
            stream = it->second;
    }

    if (!stream) [[unlikely]]
    {
        Er::Log::error(m_log, "Non-existent stream id {}", id);
        return Er::PropertyBag();
    }

    if (stream->type == StreamType::Ping)
        return nextPing(id, static_cast<PingStream*>(stream.get()));

    Er::Log::error(m_log, "Unknown stream type");

    return Er::PropertyBag();
}

CoreService::StreamId CoreService::beginPingStream(const Er::PropertyBag& args)
{
    auto id = m_nextStreamId++;
    auto sender = Er::getPropertyValue<Er::Server::Props::PingSender>(args);
    auto dataSize = Er::getPropertyValueOr<Er::Server::Props::PingDataSize>(args, 4096);
    
    {
        std::lock_guard l(m_mutexStreams);
        m_streams.insert({ id, std::make_shared<PingStream>(dataSize, sender) });
    }

    Er::Log::debug(m_log, "Started ping stream {}", id);

    return id;
}

Er::PropertyBag CoreService::nextPing(StreamId id, PingStream* streamData)
{
    Er::PropertyBag reply;

    auto data = generateData(seed(), streamData->dataSize);
    auto seqId = streamData->seqId++;

    Er::addProperty<Er::Server::Props::PingDataSize>(reply, streamData->dataSize);
    Er::addProperty<Er::Server::Props::PingData>(reply, std::move(data));
    Er::addProperty<Er::Server::Props::PingSequence>(reply, seqId);

    if (streamData->sender)
        Er::addProperty<Er::Server::Props::PingSender>(reply, *streamData->sender);

    Er::Log::debug(m_log, "Next ping for stream {}", id);

    return reply;
}

} // namespace Erp::Server {}