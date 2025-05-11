#pragma once

#include <protobuf/proctree.pb.h>

#include <erebus/ipc/grpc/protocol.hxx>
#include <erebus/proctree/process_props.hxx>


namespace Er::ProcessTree
{

void marshalProcessProperties(const ProcessProperties& source, erebus::ProcessProps& dest);
ProcessProperties unmarshalProcessProperties(const erebus::ProcessProps& src);

void marshalProcessPropertyMsk(erebus::ProcessPropsRequest& dest, const ProcessProperties::Mask& required);
ProcessProperties::Mask unmarshalProcessPropertyMask(const erebus::ProcessPropsRequest& req);

} // namespace Er::ProcessTree {}