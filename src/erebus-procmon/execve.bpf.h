#pragma once

#include "event_type.h"

#define DATA_SIZE 256

struct process_info_t {
    pid_t pid;
    pid_t ppid;
    __u32 uid;
    __u32 sid;
    __u64 start_time;
    __u64 retval;
    enum event_type type;
    char data[DATA_SIZE];
} __attribute__((__packed__));

#undef DATA_SIZE
