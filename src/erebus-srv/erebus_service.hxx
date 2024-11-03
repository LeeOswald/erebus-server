#pragma once


#include <erebus-srv/erebus-srv.hxx>
#include <erebus/erebus.grpc.pb.h>
#include <erebus/util/exceptionutil.hxx>

#include <atomic>
#include <shared_mutex>
#include <unordered_map>

namespace Erp::Server
{

class ErebusCbService final
    : public erebus::Erebus::CallbackService
    , public Er::Server::IServer
{
public:
    ~ErebusCbService();
    ErebusCbService(const Er::Server::Params& params);

    grpc::ServerUnaryReactor* GenericRpc(grpc::CallbackServerContext* context, const erebus::ServiceRequest* request, erebus::ServiceReply* reply) override;
    grpc::ServerWriteReactor<erebus::ServiceReply>* GenericStream(grpc::CallbackServerContext* context, const erebus::ServiceRequest* request) override;

    void registerService(std::string_view request, Er::Server::IService::Ptr service) override;
    void unregisterService(Er::Server::IService* service) override;

private:
    class ServiceReplyStream
        : public grpc::ServerWriteReactor<erebus::ServiceReply> 
    {
    public:
        ~ServiceReplyStream()
        {
            Er::Log::debug(m_log, "{}.ServiceReplyStream::~ServiceReplyStream", Er::Format::ptr(this));
            Er::Log::Indent idt(m_log);

            if (m_streamId)
            {
                if (m_service)
                    m_service->endStream(m_streamId);
            }
        }

        ServiceReplyStream(Er::Log::ILog* log) noexcept
            : m_log(log)
        {
            Er::Log::debug(m_log, "{}.ServiceReplyStream::ServiceReplyStream", Er::Format::ptr(this));
            Er::Log::Indent idt(m_log);
        }

        void Begin(Er::Server::IService::Ptr service, std::string_view request, std::string_view cookie, const Er::PropertyBag& args)
        {
            Er::Log::debug(m_log, "{}.ServiceReplyStream::Begin", Er::Format::ptr(this));
            Er::Log::Indent idt(m_log);

            ErAssert(!m_service);
            m_service = service;

            bool error = false;
            try
            {
                m_streamId = service->beginStream(request, cookie, args);
            }
            catch (Er::Exception& e)
            {
                Er::Util::logException(m_log, Er::Log::Level::Error, e);

                marshalException(&m_response, e);
                error = true;
            }
            catch (std::exception& e)
            {
                Er::Util::logException(m_log, Er::Log::Level::Error, e);

                marshalException(&m_response, e);
                error = true;
            }

            if (error)
                StartWriteAndFinish(&m_response, grpc::WriteOptions(), grpc::Status::OK); // just send the exception
            else
                Continue();
        }

        void OnWriteDone(bool ok) override 
        {
            Er::Log::debug(m_log, "{}.ServiceReplyStream::OnWriteDone", Er::Format::ptr(this));
            Er::Log::Indent idt(m_log);

            if (!ok) 
                Finish(grpc::Status(grpc::StatusCode::UNKNOWN, "Unexpected Failure"));
            else
                Continue();
        }

        void OnDone() override 
        {
            Er::Log::debug(m_log, "{}.ServiceReplyStream::OnDone", Er::Format::ptr(this));
            Er::Log::Indent idt(m_log);

            delete this;
        }

        void OnCancel() override 
        {
            Er::Log::debug(m_log, "{}.ServiceReplyStream::OnCancel", Er::Format::ptr(this));
            Er::Log::Indent idt(m_log);
        }

    private:
        void Continue()
        {
            Er::Log::debug(m_log, "{}.ServiceReplyStream::Continue", Er::Format::ptr(this));
            Er::Log::Indent idt(m_log);

            m_response.Clear();
            bool error = false;

            try
            {
                auto item = m_service->next(m_streamId);
                if (item.empty())
                {
                    // end of stream
                    Finish(grpc::Status::OK);
                    return;
                }
                else
                {
                    marshalReplyProps(item, &m_response);
                }
            }
            catch (Er::Exception& e)
            {
                Er::Util::logException(m_log, Er::Log::Level::Error, e);

                marshalException(&m_response, e);
                error = true;
            }
            catch (std::exception& e)
            {
                Er::Util::logException(m_log, Er::Log::Level::Error, e);

                marshalException(&m_response, e);
                error = true;
            }

            if (error)
                StartWriteAndFinish(&m_response, grpc::WriteOptions(), grpc::Status::OK); // just send the exception
            else
                StartWrite(&m_response, grpc::WriteOptions().set_buffer_hint());
        }

        
        Er::Log::ILog* m_log;
        Er::Server::IService::Ptr m_service;
        Er::Server::IService::StreamId m_streamId = {};
        erebus::ServiceReply m_response;
    };

    Er::Server::IService::Ptr findService(const std::string& id) const;
    static Er::PropertyBag unmarshalArgs(const erebus::ServiceRequest* request);
    static void marshalReplyProps(const Er::PropertyBag& props, erebus::ServiceReply* reply);
    static void marshalException(erebus::ServiceReply* reply, const std::exception& e);
    static void marshalException(erebus::ServiceReply* reply, const Er::Exception& e);

    Er::Server::Params m_params;
    std::unique_ptr<grpc::Server> m_server;
    mutable std::shared_mutex m_servicesLock;
    std::unordered_map<std::string, Er::Server::IService::Ptr> m_services;
};

} // namespace Erp::Server {}