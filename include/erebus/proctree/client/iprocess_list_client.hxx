#pragma once

#include <erebus/ipc/grpc/client/iclient.hxx>
#include <erebus/proctree/process_props.hxx>
#include <erebus/rtl/log.hxx>
#include <erebus/rtl/time.hxx>


namespace Er::ProcessTree
{


struct IProcessListClient
    : public Ipc::Grpc::IClient
{
    static constexpr std::string_view IID = "Er.ProcessTree.IProcessListClient";
    
    struct Timings
    {
        Time rtt;          // round-trip time
        Time processing;   // actual server processing time 
    };

    struct IGetProcessPropsCompletion
        : public IClient::ICompletion
    {
        virtual void onReply(ProcessProperties&& props, Timings timings) = 0;

    protected:
        virtual ~IGetProcessPropsCompletion() = default;
    };

    using GetProcessPropsCompletionPtr = ReferenceCountedPtr<IGetProcessPropsCompletion>;

    virtual void getProcessProperties(Pid pid, const ProcessProperties::Mask& required, GetProcessPropsCompletionPtr completion) = 0;
};

using ProcessListClientPtr = ReferenceCountedPtr<IProcessListClient>;


[[nodiscard]] ER_PROCTREE_EXPORT ProcessListClientPtr createProcessListClient(Ipc::Grpc::ChannelPtr channel, Log::LoggerPtr log);

} // namespace Er::ProcessTree {}