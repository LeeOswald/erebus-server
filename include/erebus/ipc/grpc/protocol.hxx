#pragma once

#include <protobuf/common.pb.h>

#include <erebus/rtl/property_bag.hxx>


namespace Er::Ipc::Grpc
{

void marshalProperty(const Property& source, erebus::Property& dest);

} // namespace Er::Ipc::Grpc {}