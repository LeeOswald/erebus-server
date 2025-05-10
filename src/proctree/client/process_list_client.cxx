#include <erebus/proctree/client/iprocess_list_client.hxx>


namespace Er::ProcessTree
{

ER_PROCTREE_EXPORT ProcessListClientPtr createProcessListClient(Ipc::Grpc::ChannelPtr channel, Log::LoggerPtr log)
{
    return {};
}


} // namespace Er::ProcessTree {}
