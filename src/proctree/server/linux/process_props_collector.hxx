#pragma once

#include <erebus/proctree/process_props.hxx>
#include <erebus/proctree/server/linux/procfs.hxx>

#include <erebus/rtl/log.hxx>

namespace Er::ProcessTree::Linux
{

std::expected<ProcessProperties, Error> collectProcessProps(Linux::ProcFs& procFs, Pid pid, const ProcessProperties::Mask& mask, Log::ILogger* log);

} // namespace Er::ProcessTree::Linux {}