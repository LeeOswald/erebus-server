#pragma once

#include <erebus/proctree/linux/procfs.hxx>
#include <erebus/proctree/process_props.hxx>
#include <erebus/rtl/log.hxx>

namespace Er::ProcessTree::Linux
{

std::expected<ProcessProperties, int> collectProcessProps(Linux::ProcFs& procFs, Pid pid, const ProcessProperties::Mask& mask, Log::ILogger* log);

} // namespace Er::ProcessTree::Linux {}