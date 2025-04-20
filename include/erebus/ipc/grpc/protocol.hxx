#pragma once

#include <grpcpp/grpcpp.h>
#include <protobuf/common.pb.h>

#include <erebus/rtl/exception.hxx>
#include <erebus/rtl/property_bag.hxx>


namespace Er::Ipc::Grpc
{

void marshalProperty(const Property& source, erebus::Property& dest);
Property unmarshalProperty(const erebus::Property& source);

void marshalException(const Exception& source, erebus::Exception& dest);
Exception unmarshalException(const erebus::Exception& source);

Er::ResultCode mapGrpcStatus(grpc::StatusCode status) noexcept;
grpc::StatusCode resultToGrpcStatus(Er::ResultCode code) noexcept;


} // namespace Er::Ipc::Grpc {}