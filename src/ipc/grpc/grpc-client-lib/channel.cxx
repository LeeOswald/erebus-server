#include <grpcpp/grpcpp.h>

#include <erebus/ipc/grpc/client/grpc_client.hxx>
#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/util/file.hxx>


namespace Er::Ipc::Grpc
{

ChannelPtr createChannel(const PropertyMap& parameters)
{
    auto prop = findProperty(parameters, "endpoint", Property::Type::String);
    if (!prop)
        throw Exception(std::source_location::current(), Error(Result::BadConfiguration, GenericError), Exception::Message("Endpoint address expected"));

    std::string endpoint = *prop->getString();

    bool keepalive = false;
    prop = findProperty(parameters, "keepalive", Property::Type::Bool);
    if (prop)
        keepalive = *prop->getBool();

    bool tls = false;
    prop = findProperty(parameters, "tls", Property::Type::Bool);
    if (prop)
        tls = *prop->getBool();

    std::string certificate;
    std::string key;
    std::string rootCerts;

    if (tls)
    {
        prop = findProperty(parameters, "certificate", Property::Type::String);
        if (!prop)
            throw Exception(std::source_location::current(), Error(Result::BadConfiguration, GenericError), Exception::Message("TLS certificate file name expected"));
        
        certificate = Util::loadFile(*prop->getString()).release();

        prop = findProperty(parameters, "private_key", Property::Type::String);
        if (!prop)
            throw Exception(std::source_location::current(), Error(Result::BadConfiguration, GenericError), Exception::Message("TLS private key file name expected"));

        key = Util::loadFile(*prop->getString()).release();

        prop = findProperty(parameters, "root_certificates", Property::Type::String);
        if (!prop)
            throw Exception(std::source_location::current(), Error(Result::BadConfiguration, GenericError), Exception::Message("TLS root certificates file name expected"));

        rootCerts = Util::loadFile(*prop->getString()).release();
    }

    grpc::ChannelArguments args;

    if (keepalive)
    {
        args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 20 * 1000);
        args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 10 * 1000);
        args.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
    }

    if (tls)
    {
        grpc::SslCredentialsOptions opts;
        opts.pem_root_certs = rootCerts;
        opts.pem_cert_chain = certificate;
        opts.pem_private_key = key;

        auto channelCreds = grpc::SslCredentials(opts);
        return grpc::CreateCustomChannel(endpoint, channelCreds, args);
    }
    else
    {
        return grpc::CreateCustomChannel(endpoint, grpc::InsecureChannelCredentials(), args);
    }
}

} // namespace Er::Ipc::Grpc {}