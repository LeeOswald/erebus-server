#pragma once

#include <erebus-srv/erebus-srv.hxx>

#include <grpcpp/grpcpp.h>
#include <google/protobuf/message.h>

#include <functional>
#include <list>


namespace Erp
{

namespace Server
{


namespace Rpc
{

using TagProcessor = std::function<void(bool)>;

struct TagInfo
{
    TagProcessor* tagProcessor = nullptr; // the function to be called to process incoming event
    bool ok = false;                      // the result of tag processing as indicated by gRPC library

    constexpr TagInfo() noexcept = default;
};

using TagList = std::list<TagInfo>;


class RpcBase
{
public:
    virtual ~RpcBase()
    {
    };

    RpcBase()
        : m_asyncOpCounter(0)
        , m_asyncReadInProgress(false)
        , m_asyncWriteInProgress(false)
        , m_onDoneCalled(false)
    {
    }

    const grpc::ServerContext& getServerContext() const
    {
        return m_serverContext;
    }

    bool sendResponse(const google::protobuf::Message* response)
    {
        return sendResponseImpl(response);
    }

    // this should be called for system level errors when no response is available
    bool finishWithError(const grpc::Status& error)
    {
        return finishWithErrorImpl(error);
    }

    // Tag processor for the 'done' event of this rpc from gRPC library
    void onDone(bool /*ok*/)
    {
        m_onDoneCalled = true;
        if (m_asyncOpCounter == 0)
            done();
    }

    // Each different rpc type need to implement the specialization of action when this rpc is done.
    virtual void done() = 0;

protected:
    enum class AsyncOpType
    {
        Invalid,
        RequestQueued,
        Read,
        Write,
        Finish
    };

    virtual bool sendResponseImpl(const google::protobuf::Message* response) = 0;
    virtual bool finishWithErrorImpl(const grpc::Status& error) = 0;

    void asyncOpStarted(AsyncOpType opType)
    {
        ++m_asyncOpCounter;

        switch (opType)
        {
        case AsyncOpType::Read:
            m_asyncReadInProgress = true;
            break;
        case AsyncOpType::Write:
            m_asyncWriteInProgress = true;
        default:
            break;
        }
    }

    // returns true if the rpc processing should keep going. false otherwise.
    bool asyncOpFinished(AsyncOpType opType)
    {
        --m_asyncOpCounter;

        switch (opType)
        {
        case AsyncOpType::Read:
            m_asyncReadInProgress = false;
            break;
        case AsyncOpType::Write:
            m_asyncWriteInProgress = false;
        default:
            break;
        }

        // No async operations are pending and gRPC library notified as earlier that it is done with the rpc.
        // Finish the rpc. 
        if (m_asyncOpCounter == 0 && m_onDoneCalled)
        {
            done();
            return false;
        }

        return true;
    }

    bool asyncOpInProgress() const
    {
        return m_asyncOpCounter != 0;
    }

    bool asyncReadInProgress() const
    {
        return m_asyncReadInProgress;
    }

    bool asyncWriteInProgress() const
    {
        return m_asyncWriteInProgress;
    }

    // The application can use the ServerContext for taking into account the current 'situation' of the rpc.
    grpc::ServerContext m_serverContext;

private:
    int32_t m_asyncOpCounter;
    bool m_asyncReadInProgress;
    bool m_asyncWriteInProgress;

    // In case of an abrupt rpc ending (for example, client process exit), gRPC calls OnDone prematurely even while an async operation is in progress
    // and would be notified later. An example sequence would be
    // 1. The client issues an rpc request. 
    // 2. The server handles the rpc and calls Finish with response. At this point, ServerContext::IsCancelled is NOT true.
    // 3. The client process abruptly exits. 
    // 4. The completion queue dispatches an OnDone tag followed by the OnFinish tag. If the application cleans up the state in OnDone, OnFinish invocation would result in undefined behavior. 
    // This actually feels like a pretty odd behavior of the gRPC library (it is most likely a result of our multi-threaded usage) so we account for that by keeping track of whether the OnDone was called earlier. 
    // As far as the application is considered, the rpc is only 'done' when no async Ops are pending. 
    bool m_onDoneCalled;
};


template <typename ServiceTypeT, typename RequestTypeT, typename ResponseTypeT>
struct RpcHandlers
{
    using CreateRpc = std::function<void(grpc::Service*, grpc::ServerCompletionQueue*)>;
    using ProcessIncomingRequest = std::function<void(RpcBase&, const google::protobuf::Message*)>;
    using Done = std::function<void(RpcBase&, bool)>;

    // In gRPC async model, an application has to explicitly ask the gRPC server to start handling an incoming rpc on a particular service.
    // createRpc is called when an outstanding RpcBase starts serving an incoming rpc and we need to create the next rpc of this type to service 
    // further incoming rpcs.
    CreateRpc createRpc;

    // A new request has come in for this rpc. processIncomingRequest is called to handle it. Note that with streaming rpcs, a request can 
    // come in multiple times. 
    ProcessIncomingRequest processIncomingRequest;

    // The gRPC server is done with this Rpc. Any necessary clean up can be done when done is called. 
    Done done;
};


template <typename ServiceTypeT, typename RequestTypeT, typename ResponseTypeT>
struct UnaryRpcHandlers
    : public RpcHandlers<ServiceTypeT, RequestTypeT, ResponseTypeT>
{
    using ResponseWriter = grpc::ServerAsyncResponseWriter<ResponseTypeT>;
    using RequestRpc = std::function<void(ServiceTypeT*, grpc::ServerContext*, RequestTypeT*, ResponseWriter*, grpc::CompletionQueue*, grpc::ServerCompletionQueue*, void*)>;

    // The actual queuing function on the generated service. This is called when an instance of rpc job is created. 
    RequestRpc requestRpc;
};


template<typename ServiceTypeT, typename RequestTypeT, typename ResponseTypeT>
struct ServerStreamingRpcHandlers 
    : public RpcHandlers<ServiceTypeT, RequestTypeT, ResponseTypeT>
{
    using ResponseWriter = grpc::ServerAsyncWriter<ResponseTypeT>;
    using RequestRpc = std::function<void(ServiceTypeT*, grpc::ServerContext*, RequestTypeT*, ResponseWriter*, grpc::CompletionQueue*, grpc::ServerCompletionQueue*, void*)>;

    // The actual queuing function on the generated service. This is called when an instance of rpc job is created. 
    RequestRpc requestRpc;
};



template <typename ServiceTypeT, typename RequestTypeT, typename ResponseTypeT>
class UnaryRpc
    : public RpcBase
{
private:
    using ThisRpcTypeJobHandlers = UnaryRpcHandlers<ServiceTypeT, RequestTypeT, ResponseTypeT>;

public:
    UnaryRpc(ServiceTypeT* service, grpc::ServerCompletionQueue* cq, ThisRpcTypeJobHandlers jobHandlers)
        : m_service(service)
        , m_cq(cq)
        , m_responseWriter(&m_serverContext)
        , m_handlers(jobHandlers)
    {
        // create TagProcessors that we'll use to interact with gRPC CompletionQueue
        m_onRead = std::bind(&UnaryRpc::onRead, this, std::placeholders::_1);
        m_onFinish = std::bind(&UnaryRpc::onFinish, this, std::placeholders::_1);
        m_onDone = std::bind(&RpcBase::onDone, this, std::placeholders::_1);

        // set up the completion queue to inform us when gRPC is done with this rpc.
        m_serverContext.AsyncNotifyWhenDone(&m_onDone);

        // finally, issue the async request needed by gRPC to start handling this rpc.
        asyncOpStarted(RpcBase::AsyncOpType::RequestQueued);
        m_handlers.requestRpc(m_service, &m_serverContext, &m_request, &m_responseWriter, m_cq, m_cq, &m_onRead);
    }

private:
    bool sendResponseImpl(const google::protobuf::Message* responseMsg) override
    {
        auto response = static_cast<const ResponseTypeT*>(responseMsg);

        ErAssert(response); // If no response is available, use RpcBase::finishWithError.

        m_response = *response;

        asyncOpStarted(RpcBase::AsyncOpType::Finish);
        m_responseWriter.Finish(m_response, grpc::Status::OK, &m_onFinish);

        return true;
    }

    bool finishWithErrorImpl(const grpc::Status& error) override
    {
        asyncOpStarted(RpcBase::AsyncOpType::Finish);
        m_responseWriter.FinishWithError(error, &m_onFinish);

        return true;
    }

    void onRead(bool ok)
    {
        // A request has come on the service which can now be handled. Create a new rpc of this type to allow the server to handle next request.
        m_handlers.createRpc(m_service, m_cq);

        if (asyncOpFinished(RpcBase::AsyncOpType::RequestQueued))
        {
            if (ok)
            {
                // We have a request that can be responded to now. So process it. 
                m_handlers.processIncomingRequest(*this, &m_request);
            }
            else
            {
                ErAssert(ok);
            }
        }
    }

    void onFinish(bool ok)
    {
        asyncOpFinished(RpcBase::AsyncOpType::Finish);
    }

    void done() override
    {
        m_handlers.done(*this, m_serverContext.IsCancelled());
    }

    ServiceTypeT* m_service;
    grpc::ServerCompletionQueue* m_cq;
    typename ThisRpcTypeJobHandlers::ResponseWriter m_responseWriter;

    RequestTypeT m_request;
    ResponseTypeT m_response;

    ThisRpcTypeJobHandlers m_handlers;

    TagProcessor m_onRead;
    TagProcessor m_onFinish;
    TagProcessor m_onDone;
};


template <typename ServiceTypeT, typename RequestTypeT, typename ResponseTypeT>
class ServerStreamingRpc 
    : public RpcBase
{
private:
    using ThisRpcTypeJobHandlers = ServerStreamingRpcHandlers<ServiceTypeT, RequestTypeT, ResponseTypeT>;

public:
    ServerStreamingRpc(ServiceTypeT* service, grpc::ServerCompletionQueue* cq, ThisRpcTypeJobHandlers jobHandlers)
        : m_service(service)
        , m_cq(cq)
        , m_responseWriter(&m_serverContext)
        , m_handlers(jobHandlers)
        , m_serverStreamingDone(false)
    {
        // create TagProcessors that we'll use to interact with gRPC CompletionQueue
        m_onRead = std::bind(&ServerStreamingRpc::onRead, this, std::placeholders::_1);
        m_onWrite = std::bind(&ServerStreamingRpc::onWrite, this, std::placeholders::_1);
        m_onFinish = std::bind(&ServerStreamingRpc::onFinish, this, std::placeholders::_1);
        m_onDone = std::bind(&RpcBase::onDone, this, std::placeholders::_1);

        // set up the completion queue to inform us when gRPC is done with this rpc.
        m_serverContext.AsyncNotifyWhenDone(&m_onDone);

        // finally, issue the async request needed by gRPC to start handling this rpc.
        asyncOpStarted(RpcBase::AsyncOpType::RequestQueued);
        m_handlers.requestRpc(m_service, &m_serverContext, &m_request, &m_responseWriter, m_cq, m_cq, &m_onRead);
    }

private:
    // gRPC can only do one async write at a time but that is very inconvenient from the application point of view.
    // So we buffer the response below in a queue if gRPC lib is not ready for it. 
    // The application can send a null response in order to indicate the completion of server side streaming. 
    bool sendResponseImpl(const google::protobuf::Message* responseMsg) override
    {
        auto response = static_cast<const ResponseTypeT*>(responseMsg);

        if (response)
        {
            m_responseQueue.push_back(*response);

            if (!asyncWriteInProgress())
            {
                doSendResponse();
            }
        }
        else
        {
            m_serverStreamingDone = true;

            if (!asyncWriteInProgress())
            {
                doFinish();
            }
        }

        return true;
    }

    bool finishWithErrorImpl(const grpc::Status& error) override
    {
        asyncOpStarted(RpcBase::AsyncOpType::Finish);
        m_responseWriter.Finish(error, &m_onFinish);

        return true;
    }

    void doSendResponse()
    {
        asyncOpStarted(RpcBase::AsyncOpType::Write);
        m_responseWriter.Write(m_responseQueue.front(), &m_onWrite);
    }

    void doFinish()
    {
        asyncOpStarted(RpcBase::AsyncOpType::Finish);
        m_responseWriter.Finish(grpc::Status::OK, &m_onFinish);
    }

    void onRead(bool ok)
    {
        m_handlers.createRpc(m_service, m_cq);

        if (asyncOpFinished(RpcBase::AsyncOpType::RequestQueued))
        {
            if (ok)
            {
                m_handlers.processIncomingRequest(*this, &m_request);
            }
        }
    }

    void onWrite(bool ok)
    {
        if (asyncOpFinished(RpcBase::AsyncOpType::Write))
        {
            // Get rid of the message that just finished.
            m_responseQueue.pop_front();

            if (ok)
            {
                if (!m_responseQueue.empty()) // If we have more messages waiting to be sent, send them.
                {
                    doSendResponse();
                }
                else if (m_serverStreamingDone) // Previous write completed and we did not have any pending write. 
                {                               // If the application has finished streaming responses, finish the rpc processing.
                    doFinish();
                }
            }
        }
    }

    void onFinish(bool ok)
    {
        asyncOpFinished(RpcBase::AsyncOpType::Finish);
    }

    void done() override
    {
        m_handlers.done(*this, m_serverContext.IsCancelled());
    }

    ServiceTypeT* m_service;
    grpc::ServerCompletionQueue* m_cq;
    typename ThisRpcTypeJobHandlers::ResponseWriter m_responseWriter;

    RequestTypeT m_request;

    ThisRpcTypeJobHandlers m_handlers;

    TagProcessor m_onRead;
    TagProcessor m_onWrite;
    TagProcessor m_onFinish;
    TagProcessor m_onDone;

    std::list<ResponseTypeT> m_responseQueue;
    bool m_serverStreamingDone;
};




} // namespace Rpc {}

} // namespace Server {}

} // namespace Erp {}
