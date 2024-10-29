#include "svcbase.hxx"

#include <erebus/protocol.hxx>
#include <erebus/system/thread.hxx>

#include <erebus/trace.hxx>

namespace Erp::Server
{


ServiceBase::~ServiceBase()
{
    TraceMethod("ServiceBase");

    m_server->Shutdown();
    
    m_receiver.reset();
    m_handlers.clear();
}

ServiceBase::ServiceBase(grpc::Service* service, const Er::Server::Params& params)
    : m_service(service)
    , m_params(params)
{
    TraceMethod("ServiceBase");
}

void ServiceBase::start()
{
    TraceMethod("ServiceBase");
    grpc::ServerBuilder builder;

    for (auto& ep : m_params.endpoints)
    {
        if (ep.ssl)
        {
            grpc::SslServerCredentialsOptions::PemKeyCertPair keycert = { ep.privateKey, ep.certificate };
            grpc::SslServerCredentialsOptions sslOps(GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY);
            sslOps.pem_root_certs = ep.certificate;
            sslOps.pem_key_cert_pairs.push_back(keycert);
            auto creds = grpc::SslServerCredentials(sslOps);
            builder.AddListeningPort(ep.endpoint, creds);
        }
        else
        {
            // no authentication
            builder.AddListeningPort(ep.endpoint, grpc::InsecureServerCredentials());
        }
    }

    if (m_params.keepAlive)
    {
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 1 * 30 * 1000);
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 60 * 1000);
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
        builder.AddChannelArgument(GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS, 5 * 1000);
        builder.AddChannelArgument(GRPC_ARG_HTTP2_MAX_PING_STRIKES, 5);
    }

    builder.RegisterService(m_service);
    
    // create completion queues
    auto cq = builder.AddCompletionQueue();
    
    // finally assemble the server
    auto server = builder.BuildAndStart();
    if (!server)
        ErThrow("Failed to start the service");

    m_server.swap(server);

    // create RPC handlers
    m_handlers.reserve(m_params.workerThreads);
    for (unsigned i = 0; i < m_params.workerThreads; ++i)
    {
        m_handlers.emplace_back(new RpcHandler(this, m_params.log, m_incoming));
    }

    // create RPC receiver
    m_receiver.reset(new RpcReceiver(this, m_params.log, std::move(cq), m_incoming));
}

void ServiceBase::RpcReceiver::run(std::stop_token stop)
{
    TraceMethod("ServiceBase.RpcReceiver");
    Er::System::CurrentThread::setName("RpcReceiver");

    Er::Log::writeln(log, Er::Log::Level::Debug, "RPC receiver thread started");

    owner->createRpcs(cq.get());

    Erp::Server::Rpc::TagInfo tag;
    while (!stop.stop_requested())
    {
        // Block waiting to read the next event from the completion queue. The
        // event is uniquely identified by its tag, which in this case is the
        // memory address of a CallData instance.
        // The return value of Next should always be checked. This return value
        // tells us whether there is any kind of event or CQ is shutting down.
        if (!cq->Next((void**)&tag.handler, &tag.ok))
        {
            Er::Log::writeln(log, Er::Log::Level::Debug, "No more tags in completion queue");
            break;
        }

        if (stop.stop_requested())
            break;

        Er::Log::debug(log, "Tag received {}", Er::Format::ptr(tag.handler));

        {
            std::lock_guard l(incoming.mutex);
            incoming.tags.push_back(tag);
        }

        incoming.available.notify_one();
    }

    Er::Log::writeln(log, Er::Log::Level::Debug, "RPC receiver thread exited");
}

void ServiceBase::RpcHandler::run(std::stop_token stop)
{
    TraceMethod("ServiceBase.RpcHandler");
    Er::System::CurrentThread::setName("RpcHandler");

    Er::Log::writeln(log, Er::Log::Level::Debug, "RPC handler thread started");

    while (!stop.stop_requested())
    {
        decltype(incoming.tags) tags;

        {
            std::unique_lock l(incoming.mutex);

            incoming.available.wait(l, [this, &stop]() { return stop.stop_requested() || !incoming.tags.empty(); });

            if (stop.stop_requested())
                break;

            if (incoming.tags.empty())
                continue;

            auto tag = incoming.tags.front();
            incoming.tags.pop_front();
            tags.push_back(tag);
        }

        while (!stop.stop_requested() && !tags.empty())
        {
            auto tag = tags.front();
            tags.pop_front();

            Er::Log::debug(log, "Stole tag {}", Er::Format::ptr(tag.handler));

            (*(tag.handler))(tag.ok);
        }
    }

    Er::Log::writeln(log, Er::Log::Level::Debug, "RPC handler thread exited");
}

void ServiceBase::genericDone(Erp::Server::Rpc::RpcBase& rpc, bool rpcCancelled)
{
    TraceFunction();
    delete (&rpc);
}

void ServiceBase::marshalException(erebus::ServiceReply* reply, const std::exception& e)
{
    auto what = e.what();
    if (!what || !*what)
        what = "Unknown exception";

    auto exception = reply->mutable_exception();
    *exception->mutable_message() = std::string_view(what);
}

void ServiceBase::marshalException(erebus::ServiceReply* reply, const Er::Exception& e)
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

void ServiceBase::marshalException(erebus::ServiceReply* reply, Er::Result code, std::string_view message)
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