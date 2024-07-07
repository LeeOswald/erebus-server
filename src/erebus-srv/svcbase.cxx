#include "svcbase.hxx"

#include <erebus/protocol.hxx>
#include <erebus/system/thread.hxx>
#include <erebus/util/format.hxx>

namespace Er
{

namespace Server
{

namespace Private
{

ServiceBase::~ServiceBase()
{
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

ServiceBase::ServiceBase(const Params* params)
    : m_params(*params)
    , m_local(params->endpoint.starts_with("unix:"))
{

}

void ServiceBase::start()
{
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

    // register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service
    builder.RegisterService(service());
    m_queue = builder.AddCompletionQueue();

#if !ER_DEBUG
    if (!m_local)
    {
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 1 * 60 * 1000);
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 20 * 1000);
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
        builder.AddChannelArgument(GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS, 10 * 1000);
        builder.AddChannelArgument(GRPC_ARG_HTTP2_MAX_PING_STRIKES, 5);
    }
#endif

    // finally assemble the server
    auto server = builder.BuildAndStart();
    if (!server)
        throw Er::Exception(ER_HERE(), "Failed to start the service");

    m_server.swap(server);

    m_receiverWorker.reset(new std::thread([this]() { handleRpcs(); }));
    m_processorWorker.reset(new std::thread([this]() { processRpcs(); }));
}

void ServiceBase::handleRpcs()
{
    Er::System::CurrentThread::setName("RPCHandler");

    Er::Log::Debug(m_params.log, ErLogComponent("ServiceBase")) << "RPC handler thread started";

    createRpcs();

    Er::Server::Private::Rpc::TagInfo tagInfo;
    while (!m_stop)
    {
        // Block waiting to read the next event from the completion queue. The
        // event is uniquely identified by its tag, which in this case is the
        // memory address of a CallData instance.
        // The return value of Next should always be checked. This return value
        // tells us whether there is any kind of event or cq_ is shutting down.
        if (!m_queue->Next((void**)&tagInfo.tagProcessor, &tagInfo.ok))
        {
            if (!m_stop)
            {
                m_params.log->write(Er::Log::Level::Warning, ErLogComponent("ServiceBase"), "No more tags in completion queue");
                break;
            }
        }

        {
            std::lock_guard l(m_mutex);
            m_incomingTags.push_back(tagInfo);
        }

        m_incoming.notify_one();
    }

    Er::Log::Debug(m_params.log, ErLogComponent("ServiceBase")) << "RPC handler thread exited";
}

void ServiceBase::processRpcs()
{
    Er::System::CurrentThread::setName("RPCProcessor");
    Er::Log::Debug(m_params.log, ErLogComponent("ServiceBase")) << "RPC processor thread started";

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

    Er::Log::Debug(m_params.log, ErLogComponent("ServiceBase")) << "RPC processor thread exited";
}

void ServiceBase::genericDone(Er::Server::Private::Rpc::RpcBase& rpc, bool rpcCancelled)
{
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

void ServiceBase::marshalException(erebus::GenericReply* reply, Result code, std::string_view message)
{
    auto exception = reply->mutable_exception();
    
    if (!message.empty())
        *exception->mutable_message() = message;

    auto mutableProps = exception->mutable_props();
    auto mutableProp = mutableProps->Add();

    Er::Protocol::assignProperty(*mutableProp, Er::Property(ExceptionProps::ResultCode(static_cast<int32_t>(code))));
}

Er::PropertyBag ServiceBase::unmarshalArgs(const erebus::ServiceRequest* request)
{
    Er::PropertyBag bag;

    int count = request->args_size();
    for (int i = 0; i < count; ++i)
    {
        auto& arg = request->args(i);
        Er::insertProperty(bag, Er::Protocol::getProperty(arg));
    }

    return bag;
}

void ServiceBase::marshalReplyProps(const Er::PropertyBag& props, erebus::ServiceReply* reply)
{
    if (props.empty())
        return;

    auto out = reply->mutable_props();
    Er::enumerateProperties(props, [&out](const Property& prop)
    {
        auto mutableProp = out->Add();
        Er::Protocol::assignProperty(*mutableProp, prop);
    });
}

} // namespace Private {}

} // namespace Server {}

} // namespace Er {}