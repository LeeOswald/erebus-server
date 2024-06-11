#pragma once

#include "common.hxx"

void dumpProcess(Er::Log::ILog* log, const Er::Client::Params& params, int pid, int interval);

void dumpProcesses(Er::Log::ILog* log, const Er::Client::Params& params, int interval);

void dumpProcessesDiff(Er::Log::ILog* log, const Er::Client::Params& params, int interval);


void killProcess(Er::Log::ILog* log, const Er::Client::Params& params, uint64_t pid, const std::string& signame);

