#pragma once

#include <protobuf/proctree.pb.h>

#include <erebus/ipc/grpc/protocol.hxx>
#include <erebus/proctree/process_props.hxx>


namespace Er::ProcessTree
{

void marshalProcessProperties(const ProcessProperties& source, erebus::ProcessProps& dest);

ProcessProperties::Mask unmarshalProcessPropertyMask(const erebus::ProcessPropsRequest& req);

} // namespace Er::ProcessTree {}