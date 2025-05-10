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


template <typename... _Properties>
        requires (std::is_base_of_v<Property, std::remove_cvref_t<_Properties>> && ...)
void marshalError(const Error& source, erebus::Exception& dest, _Properties... props)
{
    Exception e(std::source_location::current(), source, std::forward<_Properties>(props)...);
    e.decode();

    marshalException(e, dest);
}

Exception unmarshalException(const erebus::Exception& source);


} // namespace Er::Ipc::Grpc {}