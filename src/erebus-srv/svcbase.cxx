#include "svcbase.hxx"

#include <erebus/protocol.hxx>
#include <erebus/system/thread.hxx>

#include <erebus/trace.hxx>

namespace Erp::Server
{


ServiceBase::~ServiceBase()
{
    TraceMethod("ServiceBase");
    m_stop = true;
    m_incoming.notify_one();

    m_server->Shutdown();
    m_queue->Shutdown();

    // drain the CQ
    void* ignoredTag = nullptr;
    bool ignoredOk = false;
    while (m_queue->Next(&ignoredTag, &ignoredOk))
    {
    }

    if (m_receiverWorker->joinable())
        m_receiverWorker->join();

    if (m_processorWorker->joinable())
        m_processorWorker->join();
}

ServiceBase::ServiceBase(const Er::Server::Params* params)
    : m_params(*params)
    , m_local(params->endpoint.starts_with("unix:"))
{
    TraceMethod("ServiceBase");
}

void ServiceBase::start()
{
    TraceMethod("ServiceBase");
    grpc::ServerBuilder builder;

    if (!m_local && m_params.ssl)
    {
        grpc::SslServerCredentialsOptions::PemKeyCertPair keycert = { m_params.key, m_params.certificate };
        grpc::SslServerCredentialsOptions sslOps(GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY);
        sslOps.pem_root_certs = m_params.rootCertificate;
        sslOps.pem_key_cert_pairs.push_back(keycert);
        auto creds = grpc::SslServerCredentials(sslOps);
        builder.AddListeningPort(m_params.endpoint, creds);
    }
    else
    {
        // no authentication
        builder.AddListeningPort(m_params.endpoint, grpc::InsecureServerCredentials());
    }

    builder.RegisterService(service());
    m_queue = builder.AddCompletionQueue();

    if (!m_local && !m_params.noKeepAlive)
    {
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 1 * 60 * 1000);
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 20 * 1000);
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
        builder.AddChannelArgument(GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS, 10 * 1000);
        builder.AddChannelArgument(GRPC_ARG_HTTP2_MAX_PING_STRIKES, 5);
    }

    // finally assemble the server
    auto server = builder.BuildAndStart();
    if (!server)
        ErThrow("Failed to start the service");

    m_server.swap(server);

    m_receiverWorker.reset(new std::thread([this]() { receiveRpcs(); }));
    m_processorWorker.reset(new std::thread([this]() { processRpcs(); }));
}

void ServiceBase::receiveRpcs()
{
    TraceMethod("ServiceBase");
    Er::System::CurrentThread::setName("RPCReceiver");

    Er::Log::writeln(m_params.log, Er::Log::Level::Debug, "RPC receiver thread started");

    createRpcs();

    Erp::Server::Rpc::TagInfo tagInfo;
    while (!m_stop)
    {
        // Block waiting to read the next event from the completion queue. The
        // event is uniquely identified by its tag, which in this case is the
        // memory address of a CallData instance.
        // The return value of Next should always be checked. This return value
        // tells us whether there is any kind of event or cq_ is shutting down.
        if (!m_queue->Next((void**)&tagInfo.tagProcessor, &tagInfo.ok))
        {
            Er::Log::writeln(m_params.log, Er::Log::Level::Debug, "No more tags in completion queue");
            break;
        }

        if (m_stop)
            break;

        {
            std::lock_guard l(m_mutex);
            m_incomingTags.push_back(tagInfo);
        }

        m_incoming.notify_one();
    }

    Er::Log::writeln(m_params.log, Er::Log::Level::Debug, "RPC receiver thread exited");
}

void ServiceBase::processRpcs()
{
    TraceMethod("ServiceBase");
    Er::System::CurrentThread::setName("RPCProcessor");
    Er::Log::writeln(m_params.log, Er::Log::Level::Debug, "RPC processor thread started");

    while (!m_stop)
    {
        std::unique_lock l(m_mutex);

        m_incoming.wait(l, [this]() { return m_stop || !m_incomingTags.empty(); });

        if (m_stop)
            break;

        auto tags = std::move(m_incomingTags);
        l.unlock();

        while (!tags.empty())
        {
            auto tagInfo = tags.front();
            tags.pop_front();
            (*(tagInfo.tagProcessor))(tagInfo.ok);

        }
    }

    Er::Log::writeln(m_params.log, Er::Log::Level::Debug, "RPC processor thread exited");
}

void ServiceBase::genericDone(Erp::Server::Rpc::RpcBase& rpc, bool rpcCancelled)
{
    TraceFunction();
    delete (&rpc);
}

void ServiceBase::marshalException(erebus::GenericReply* reply, const std::exception& e)
{
    auto what = e.what();
    if (!what || !*what)
        what = "Unknown exception";

    auto exception = reply->mutable_exception();
    *exception->mutable_message() = std::string_view(what);
}

void ServiceBase::marshalException(erebus::GenericReply* reply, const Er::Exception& e)
{
    std::string_view what;
    auto msg = e.message();
    if (msg)
        what = *msg;
    else
        what = "Unknown exception";

    auto exception = reply->mutable_exception();
    *exception->mutable_message() = what;

    auto properties = e.properties();
    if (properties && !properties->empty())
    {
        auto mutableProps = exception->mutable_props();
        mutableProps->Reserve(properties->size());

        for (auto& property : *properties)
        {
            auto mutableProp = mutableProps->Add();
            Er::Protocol::assignProperty(*mutableProp, property);
        }
    }
}

void ServiceBase::marshalException(erebus::GenericReply* reply, Er::Result code, std::string_view message)
{
    auto exception = reply->mutable_exception();
    
    if (!message.empty())
        *exception->mutable_message() = message;

    auto mutableProps = exception->mutable_props();
    auto mutableProp = mutableProps->Add();

    Er::Protocol::assignProperty(*mutableProp, Er::Property(Er::ExceptionProps::ResultCode(static_cast<int32_t>(code))));
}

Er::PropertyBag ServiceBase::unmarshalArgs(const erebus::ServiceRequest* request)
{
    Er::PropertyBag bag;

    int count = request->args_size();
    if (count > 0)
    {
        Er::reservePropertyBag(bag, count);
        for (int i = 0; i < count; ++i)
        {
            auto& arg = request->args(i);
            Er::addProperty(bag, Er::Protocol::getProperty(arg));
        }
    }

    return bag;
}

void ServiceBase::marshalReplyProps(const Er::PropertyBag& props, erebus::ServiceReply* reply)
{
    if (props.empty())
        return;

    auto out = reply->mutable_props();
    Er::enumerateProperties(props, [&out](const Er::Property& prop)
    {
        auto mutableProp = out->Add();
        Er::Protocol::assignProperty(*mutableProp, prop);
    });
}

} // namespace Erp::Server {}