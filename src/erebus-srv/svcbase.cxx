#include "svcbase.hxx"

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
    , m_authProcessor(std::make_shared<AuthMetadataProcessor>(params->log))
{

}

void ServiceBase::start()
{
    grpc::ServerBuilder builder;

    if (!m_local && m_params.ssl)
    {
        grpc::SslServerCredentialsOptions::PemKeyCertPair keycert = { m_params.key, m_params.certificate };
        grpc::SslServerCredentialsOptions sslOps;
        sslOps.pem_root_certs = m_params.root;
        sslOps.pem_key_cert_pairs.push_back(keycert);
        auto creds = grpc::SslServerCredentials(sslOps);
        creds->SetAuthMetadataProcessor(m_authProcessor);
        builder.AddListeningPort(m_params.endpoint, creds);
    }
    else
    {
        // listen on the given address without any authentication mechanism
        builder.AddListeningPort(m_params.endpoint, grpc::InsecureServerCredentials());
    }

    // register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service
    builder.RegisterService(service());
    m_queue = builder.AddCompletionQueue();

    builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 1 * 60 * 1000);
    builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 20 * 1000);
    builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
    builder.AddChannelArgument(GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS, 10 * 1000);
    builder.AddChannelArgument(GRPC_ARG_HTTP2_MAX_PING_STRIKES, 5);

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
    Er::Log::Debug(m_params.log) << "RPC handler thread started";

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
                m_params.log->write(Er::Log::Level::Warning, "No more tags in completion queue");
                break;
            }
        }

        {
            std::lock_guard l(m_mutex);
            m_incomingTags.push_back(tagInfo);
        }

        m_incoming.notify_one();
    }

    Er::Log::Debug(m_params.log) << "RPC handler thread exited";
}

void ServiceBase::processRpcs()
{
    Er::Log::Debug(m_params.log) << "RPC processor thread started";

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

    Er::Log::Debug(m_params.log) << "RPC processor thread exited";
}

void ServiceBase::genericDone(Er::Server::Private::Rpc::RpcBase& rpc, bool rpcCancelled)
{
    delete (&rpc);
}

std::string ServiceBase::getContextUserMapping(grpc::ServerContext* context) const
{
    return context->auth_context()->GetPeerIdentity()[0].data();
}

std::string ServiceBase::makeTicket() const
{
    Er::Util::Random r;
    return r.generate(kTicketLength, kTicketChars);
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

    auto location = e.location();
    if (location)
    {
        if (location->source)
        {
            exception->mutable_source()->set_file(location->source->file());
            exception->mutable_source()->set_line(location->source->line());
        }

        DecodedStackTrace ds;
        const DecodedStackTrace* stack = nullptr;
        if (location->decoded)
        {
            stack = &(*location->decoded);
        }
        else if (location->stack)
        {
            ds = decodeStackTrace(*location->stack);
            stack = &ds;
        }

        if (stack && !stack->empty())
        {
            auto mutableStack = exception->mutable_stack();
            for (auto& frame : *stack)
            {
                if (!frame.empty())
                    mutableStack->add_frames(frame.c_str());
                else
                    mutableStack->add_frames("???");
            }
        }
    }

    auto properties = e.properties();
    if (properties && !properties->empty())
    {
        auto mutableProps = exception->mutable_props();
        mutableProps->Reserve(properties->size());

        for (auto& property : *properties)
        {
            auto mutableProp = mutableProps->Add();
            mutableProp->set_id(property.id);

            auto info = Er::getPropertyInfo(property);
            assert(info);
            
            auto& type = info->type();
            if (type == typeid(bool))
                mutableProp->set_v_bool(std::any_cast<bool>(property.value));
            else if (type == typeid(int32_t))
                mutableProp->set_v_int32(std::any_cast<int32_t>(property.value));
            else if (type == typeid(uint32_t))
                mutableProp->set_v_uint32(std::any_cast<uint32_t>(property.value));
            else if (type == typeid(int64_t))
                mutableProp->set_v_int64(std::any_cast<int64_t>(property.value));
            else if (type == typeid(uint64_t))
                mutableProp->set_v_uint64(std::any_cast<uint64_t>(property.value));
            if (type == typeid(double))
                mutableProp->set_v_double(std::any_cast<double>(property.value));
            if (type == typeid(std::string))
                mutableProp->set_v_string(std::any_cast<std::string>(property.value));
            else
                assert(!"unsupported property type");
        }

    }
}

Er::PropertyBag ServiceBase::unmarshalArgs(const erebus::ServiceRequest* request)
{
    Er::PropertyBag bag;

    int count = request->args_size();
    for (int i = 0; i < count; ++i)
    {
        auto& arg = request->args(i);

        auto id = arg.id();
        auto info = Er::lookupProperty(id);
        if (!info)
        {
            throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported property 0x%08x", id));
        }
        else
        {
            auto& type = info->type();
            if (type == typeid(bool))
                bag.insert({ PropId(id), Property(id, arg.v_bool()) });
            else if (type == typeid(int32_t))
                bag.insert({ PropId(id), Property(id, arg.v_int32()) });
            else if (type == typeid(uint32_t))
                bag.insert({ PropId(id), Property(id, arg.v_uint32()) });
            else if (type == typeid(int64_t))
                bag.insert({ PropId(id), Property(id, arg.v_int64()) });
            else if (type == typeid(uint64_t))
                bag.insert({ PropId(id), Property(id, arg.v_uint64()) });
            else if (type == typeid(double))
                bag.insert({ PropId(id), Property(id, arg.v_double()) });
            else if (type == typeid(std::string))
                bag.insert({ PropId(id), Property(id, arg.v_string()) });
            else
                throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported property %s type %s", info->idstr(), type.name()));
        }
    }

    return bag;
}

void ServiceBase::marshalReplyProps(const Er::PropertyBag& props, erebus::ServiceReply* reply)
{
    if (props.empty())
        return;

    auto out = reply->mutable_props();
    for (auto& prop: props)
    {
        auto mutableProp = out->Add();
        mutableProp->set_id(prop.second.id);

        auto info = Er::getPropertyInfo(prop.second);
        if (!info)
            throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported property 0x%08d", prop.second.id));

        auto& type = info->type();

        if (type == typeid(bool))
            mutableProp->set_v_bool(std::any_cast<bool>(prop.second.value));
        else if (type == typeid(int32_t))
            mutableProp->set_v_int32(std::any_cast<int32_t>(prop.second.value));
        else if (type == typeid(uint32_t))
            mutableProp->set_v_uint32(std::any_cast<uint32_t>(prop.second.value));
        else if (type == typeid(int64_t))
            mutableProp->set_v_int64(std::any_cast<int64_t>(prop.second.value));
        else if (type == typeid(uint64_t))
            mutableProp->set_v_uint64(std::any_cast<uint64_t>(prop.second.value));
        else if (type == typeid(double))
            mutableProp->set_v_double(std::any_cast<double>(prop.second.value));
        else if (type == typeid(std::string))
            mutableProp->set_v_string(std::any_cast<std::string>(prop.second.value));
        else
            throw Er::Exception(ER_HERE(), Er::Util::format("Unsupported property %s type %s", info->idstr(), type.name()));
    }
}

} // namespace Private {}

} // namespace Server {}

} // namespace Er {}