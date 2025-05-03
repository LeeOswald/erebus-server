#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include <erebus/ipc/grpc/server/grpc_server.hxx>
#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/util/file.hxx>
#include <erebus/rtl/util/unknown_base.hxx>


namespace Er::Ipc::Grpc
{

namespace 
{

class ServerImpl
    : public Util::ReferenceCountedBase<Util::ObjectBase<IServer>>
{
    using Base = Util::ReferenceCountedBase<Util::ObjectBase<IServer>>;

public:
    ~ServerImpl()
    {
        ErLogDebug2(m_log.get(), "{}.ServerImpl::~ServerImpl()", Er::Format::ptr(this));

        if (m_server)
        {
            m_server->Shutdown();
            m_server.reset();
        }

        m_services.clear();

        ::grpc_shutdown();
    }

    ServerImpl(const PropertyMap& parameters, Log::LoggerPtr log)
        : m_log(log)
        , m_endpoints(parseEndpoints(parameters))
    {
        ErLogDebug2(m_log.get(), "{}.ServerImpl::ServerImpl()", Er::Format::ptr(this));

        if (m_endpoints.empty())
            ErThrow("No valid gRPC endpoints specified");

        auto keepalive = findProperty(parameters, "keepalive", Property::Type::Bool);
        if (keepalive)
            m_keepalive = *keepalive->getBool();

        ::grpc_init();
    }

    ::grpc::Server* grpc() noexcept override
    {
        return m_server.get();
    }

    void addService(ServicePtr service) override
    {
        ErAssert(service);

        if (m_server)
            ErThrow("Cannot add new services to a running server instance");

        m_services.push_back(service);

        ErLogInfo2(m_log.get(), "Service {} added", service->name());
    }

    void start() override
    {
        if (m_server)
            ErThrow("Server instance is already running");

        grpc::ServerBuilder builder;

        for (auto& ep : m_endpoints)
        {
            if (ep.tls)
            {
                ErLogInfo2(m_log.get(), "Adding server endpoint {} (TLS enabled)", ep.address);

                grpc::SslServerCredentialsOptions::PemKeyCertPair keycert = { ep.key, ep.certificate };
                grpc::SslServerCredentialsOptions sslOps(GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY);
                sslOps.pem_root_certs = ep.rootCertificates;
                sslOps.pem_key_cert_pairs.push_back(keycert);
                auto creds = grpc::SslServerCredentials(sslOps);
                builder.AddListeningPort(ep.address, creds);
            }
            else
            {
                ErLogInfo2(m_log.get(), "Adding server endpoint {} (TLS disabled)", ep.address);

                // no authentication
                builder.AddListeningPort(ep.address, grpc::InsecureServerCredentials());
            }
        }

        if (m_keepalive)
        {
            builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 1 * 30 * 1000);
            builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 60 * 1000);
            builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
            builder.AddChannelArgument(GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS, 5 * 1000);
            builder.AddChannelArgument(GRPC_ARG_HTTP2_MAX_PING_STRIKES, 5);
        }

        for (auto svc : m_services)
        {
            builder.RegisterService(svc->grpc());
        }

        // finally assemble the server
        auto server = builder.BuildAndStart();
        if (!server)
            ErThrow("Failed to start the gRPC server");

        m_server.swap(server);
    }

private:
    struct Endpoint
    {
        std::string address;
        bool tls = false;
        std::string certificate;
        std::string key;
        std::string rootCertificates;
    };

    static std::vector<Endpoint> parseEndpoints(const PropertyMap& parameters)
    {
        auto endpoints = findProperty(parameters, "endpoints", Property::Type::Vector);
        if (!endpoints || endpoints->empty())
            ErThrow("No gRPC endpoints specified");

        std::vector<Endpoint> result;

        auto v = endpoints->getVector();

        for (auto it = v->begin(); it != v->end(); ++it)
        {
            if (it->type() == Property::Type::Map)
            {
                auto m = it->getMap();
                auto address = findProperty(*m, "endpoint", Property::Type::String);
                if (!address)
                    ErThrow("Endpoint address is missing");

                Endpoint ep;
                ep.address = *address->getString();

                auto tls = findProperty(*m, "tls", Property::Type::Bool);
                if (tls && *tls->getBool())
                {
                    ep.tls = true;

                    auto certificatePath = findProperty(*m, "certificate", Property::Type::String);
                    if (!certificatePath)
                        ErThrow(Er::format("TLS certificate file path is missing for {}", ep.address));

                    auto keyPath = findProperty(*m, "private_key", Property::Type::String);
                    if (!keyPath)
                        ErThrow(Er::format("TLS private key file path is missing for {}", ep.address));

                    auto rootCertPath = findProperty(*m, "root_certificates", Property::Type::String);
                    if (!rootCertPath)
                        ErThrow(Er::format("TLS root certificates file path is missing for {}", ep.address));

                    ep.certificate = Util::loadFile(*certificatePath->getString()).release();
                    ep.key = Util::loadFile(*keyPath->getString()).release();
                    ep.rootCertificates = Util::loadFile(*rootCertPath->getString()).release();
                }

                result.push_back(std::move(ep));
            }
        }

        return result;
    }

    Log::LoggerPtr m_log;
    std::vector<Endpoint> m_endpoints;
    bool m_keepalive = false;
    std::vector<ServicePtr> m_services;
    std::unique_ptr<::grpc::Server> m_server;
};


} // namespace {}


ServerPtr createServer(const PropertyMap& parameters, Log::LoggerPtr log)
{
    return ServerPtr{ new ServerImpl(parameters, log) };
}


} // namespace Er::Ipc::Grpc {}