#pragma once


#include <erebus/rtl/log.hxx>
#include <erebus/rtl/property_bag.hxx>


namespace Er::Ipc::Grpc
{

using ChannelPtr = std::shared_ptr<void>;

[[nodiscard]] ChannelPtr createChannel(const PropertyMap& parameters);

} // namespace Er::Ipc::Grpc {}