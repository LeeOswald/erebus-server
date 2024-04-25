#pragma once

#ifdef __cplusplus
extern "C" {
#endif

enum process_event_type {
    PROCESS_EVENT_EXECVE_ENTER,
    PROCESS_EVENT_EXECVE_FILENAME,
    PROCESS_EVENT_EXECVE_ARG,
    PROCESS_EVENT_EXECVE_RETVAL,
    PROCESS_EVENT_FORK_ENTER,
    PROCESS_EVENT_FORK_RETVAL,
    PROCESS_EVENT_VFORK_ENTER,
    PROCESS_EVENT_VFORK_RETVAL,
    PROCESS_EVENT_EXIT
};


struct process_event_header_t {
    pid_t pid;
    enum process_event_type type;
} __attribute__((__packed__));


struct process_event_execve_enter_t {
    struct process_event_header_t header;
    pid_t ppid;
    __u32 uid;
    __u32 sid;
    __u64 start_time;
    char comm[16];
} __attribute__((__packed__));


struct process_event_retval_t {
    struct process_event_header_t header;
    __u64 retval;
} __attribute__((__packed__));


struct process_event_data_t {
    struct process_event_header_t header;
    char data[256];
} __attribute__((__packed__));


struct process_event_exit_t {
    struct process_event_header_t header;
    pid_t tid;
    __s32 exit_code;
} __attribute__((__packed__));


struct process_event_fork_enter_t {
    struct process_event_header_t header;
    pid_t ppid;
    __u32 uid;
    __u32 sid;
    __u64 start_time;
    char comm[16];
} __attribute__((__packed__));


#ifdef __cplusplus
} // extern "C" { }
#endif