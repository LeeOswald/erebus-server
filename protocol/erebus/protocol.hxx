#pragma once

#include <erebus/property.hxx>

#include <erebus/erebus.pb.h>



namespace Er::Protocol
{

void assignProperty(erebus::Property& out, const Property& source);
Property getProperty(const erebus::Property& source);

} // namespace Er::Protocol {}