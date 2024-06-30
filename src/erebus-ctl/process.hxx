#pragma once

#include "common.hxx"

void dumpProcess(Er::Log::ILog* log, const Er::Client::ChannelParams& params, int pid, int interval);

void dumpProcesses(Er::Log::ILog* log, const Er::Client::ChannelParams& params, int interval);

void dumpProcessesDiff(Er::Log::ILog* log, const Er::Client::ChannelParams& params, int interval);


void killProcess(Er::Log::ILog* log, const Er::Client::ChannelParams& params, uint64_t pid, const std::string& signame);

