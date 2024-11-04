#include "erebus_service.hxx"

#include <erebus/util/exceptionutil.hxx>
#include <erebus/protocol.hxx>

#include <grpcpp/grpcpp.h>

namespace Erp::Server
{

ErebusCbService::~ErebusCbService()
{
    m_server->Shutdown();
}

ErebusCbService::ErebusCbService(const Er::Server::Params& params)
    : m_params(params)
{
    grpc::ServerBuilder builder;

    for (auto& ep : m_params.endpoints)
    {
        if (ep.ssl)
        {
            grpc::SslServerCredentialsOptions::PemKeyCertPair keycert = { ep.privateKey, ep.certificate };
            grpc::SslServerCredentialsOptions sslOps(GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY);
            sslOps.pem_root_certs = ep.rootCA;
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

    builder.RegisterService(this);

    // finally assemble the server
    auto server = builder.BuildAndStart();
    if (!server)
        ErThrow("Failed to start the gRPC server");

    m_server.swap(server);
}

Er::Server::IService::Ptr ErebusCbService::findService(const std::string& id) const
{
    std::shared_lock l(m_servicesLock);

    auto it = m_services.find(id);
    if (it != m_services.end())
    {
        return it->second;
    }

    return {};
}

Er::PropertyBag ErebusCbService::unmarshalArgs(const erebus::ServiceRequest* request)
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

void ErebusCbService::marshalReplyProps(const Er::PropertyBag& props, erebus::ServiceReply* reply)
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

void ErebusCbService::marshalException(erebus::ServiceReply* reply, const std::exception& e)
{
    std::string_view what;
    auto msg = e.what();
    if (msg && *msg)
        what = msg;
    else
        what = "std::exception";

    auto exception = reply->mutable_exception();
    *exception->mutable_message() = std::string_view(e.what());
}

void ErebusCbService::marshalException(erebus::ServiceReply* reply, const Er::Exception& e)
{
    std::string_view what;
    auto msg = e.message();
    if (msg && !msg->empty())
        what = *msg;
    else
        what = "Er::Exception";

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

grpc::ServerUnaryReactor* ErebusCbService::GenericRpc(grpc::CallbackServerContext* context, const erebus::ServiceRequest* request, erebus::ServiceReply* reply)
{
    Er::Log::debug(m_params.log, "ErebusCbService::GenericRpc");
    Er::Log::Indent idt(m_params.log);

    auto reactor = context->DefaultReactor();
    if (context->IsCancelled()) [[unlikely]]
    {
        Er::Log::warning(m_params.log, "Request cancelled");
        return reactor;
    }

    auto& requestStr = request->request();
    auto service = findService(requestStr);
    if (!service) [[unlikely]]
    {
        auto msg = Er::format("No handlers for [{}]", requestStr);
        Er::Log::writeln(m_params.log, Er::Log::Level::Error, msg);
        reactor->Finish(grpc::Status(grpc::UNAVAILABLE, msg));
        return reactor;
    }

    std::string_view cookie;
    if (request->has_cookie())
        cookie = request->cookie();

    try
    {
        auto args = unmarshalArgs(request);
        auto result = service->request(requestStr, cookie, args);
        marshalReplyProps(result, reply);
    }
    catch (Er::Exception& e)
    {
        Er::Util::logException(m_params.log, Er::Log::Level::Error, e);
        marshalException(reply, e);
    }
    catch (std::exception& e)
    {
        Er::Util::logException(m_params.log, Er::Log::Level::Error, e);
        marshalException(reply, e);
    }

    reactor->Finish(grpc::Status::OK);
    return reactor;
}

grpc::ServerWriteReactor<erebus::ServiceReply>* ErebusCbService::GenericStream(grpc::CallbackServerContext* context, const erebus::ServiceRequest* request)
{
    Er::Log::debug(m_params.log, "ErebusCbService::GenericStream");
    Er::Log::Indent idt(m_params.log);

    auto reactor = std::make_unique<ServiceReplyStream>(m_params.log);

    auto& requestStr = request->request();
    auto service = findService(requestStr);
    if (!service)
    {
        auto msg = Er::format("No handlers for [{}]", requestStr);
        Er::Log::writeln(m_params.log, Er::Log::Level::Error, msg);
        reactor->Finish(grpc::Status(grpc::UNAVAILABLE, msg));
        return reactor.release();
    }

    std::string_view cookie;
    if (request->has_cookie())
        cookie = request->cookie();

    std::string errorMsg;
    try
    {
        auto args = unmarshalArgs(request);
        reactor->Begin(service, requestStr, cookie, args);
        return reactor.release();
    }
    catch (Er::Exception& e)
    {
        Er::Util::logException(m_params.log, Er::Log::Level::Error, e);
        errorMsg = e.message() ? *e.message() : "Er::Exception";
    }
    catch (std::exception& e)
    {
        Er::Util::logException(m_params.log, Er::Log::Level::Error, e);
        errorMsg = e.what() ? e.what() : "std::exception";
    }

    reactor->Finish(grpc::Status(grpc::INTERNAL, errorMsg));
    return reactor.release();
}

void ErebusCbService::registerService(std::string_view request, Er::Server::IService::Ptr service)
{
    std::lock_guard l(m_servicesLock);

    std::string id(request);
    auto it = m_services.find(id);
    if (it != m_services.end())
        ErThrow(Er::format("Service for [{}] is already registered", id));

    Er::Log::info(m_params.log, "Registered service {} for [{}]", Er::Format::ptr(service.get()), id);

    m_services.insert({ std::move(id), service });
}

void ErebusCbService::unregisterService(Er::Server::IService* service)
{
    std::lock_guard l(m_servicesLock);

    bool success = false;
    for (auto it = m_services.begin(); it != m_services.end();)
    {
        if (it->second.get() == service)
        {
            Er::Log::info(m_params.log, "Unregistered service {}", Er::Format::ptr(service));

            auto next = std::next(it);
            m_services.erase(it);
            it = next;

            success = true;
        }
        else
        {
            ++it;
        }
    }

    if (!success)
        Er::Log::error(m_params.log, "Service {} is not registered", Er::Format::ptr(service));
}

} // namespace Erp::Server {}


namespace Er::Server
{

IServer::Ptr EREBUSSRV_EXPORT create(const Params& params)
{
    return std::make_unique<Erp::Server::ErebusCbService>(params);
}

} // namespace Er::Server {}