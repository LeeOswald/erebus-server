#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

#include "process.bpf.h"

char LICENSE[] SEC("license") = "GPL v2";


#define MAX_PROCESS_ARGS 150


struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} g_ringbuf SEC(".maps");


__attribute__((always_inline)) pid_t issue_process_start(struct task_struct *task)
{
    struct process_event_start_t *ev = bpf_ringbuf_reserve(&g_ringbuf, sizeof(struct process_event_start_t), 0);
    if (!ev)
        return (pid_t)-1;

    pid_t pid = (pid_t)(bpf_get_current_pid_tgid() >> 32);
    ev->header.pid = pid;
    ev->header.type = PROCESS_EVENT_START;

    ev->ppid = (pid_t)BPF_CORE_READ(task, real_parent, tgid);
    ev->uid = bpf_get_current_uid_gid() & 0xffffffff;
    ev->sid = (__u32)BPF_CORE_READ(task, sessionid);
    ev->start_time = (__u64)BPF_CORE_READ(task, se.exec_start);

    bpf_get_current_comm(&ev->comm, sizeof(ev->comm));

    bpf_ringbuf_submit(ev, 0);
    return pid;
}

__attribute__((always_inline)) int issue_process_filename(struct trace_event_raw_sys_enter *ctx, pid_t pid)
{
    struct process_event_data_t *ev = bpf_ringbuf_reserve(&g_ringbuf, sizeof(struct process_event_data_t), 0);
    if (!ev)
        return -1;

    ev->header.pid = pid;
    ev->header.type = PROCESS_EVENT_FILENAME;

    const char* filename = (const char*)(BPF_CORE_READ(ctx, args[0]));
    if (bpf_core_read_user_str(ev->data, sizeof(ev->data), filename) <= 0)
        ev->data[0] = '\0';

    bpf_ringbuf_submit(ev, 0);
    return 0;
}

__attribute__((always_inline)) int issue_process_args(struct trace_event_raw_sys_enter *ctx, pid_t pid)
{
    const char **argv = (const char **)(BPF_CORE_READ(ctx, args[1]));
    
 #pragma clang loop unroll(full)
	for (int i = 0; i < MAX_PROCESS_ARGS; ++i)
    {
        struct process_event_data_t *ev = bpf_ringbuf_reserve(&g_ringbuf, sizeof(struct process_event_data_t), 0);
        if (!ev)
            break;

        ev->header.pid = pid;
        ev->header.type = PROCESS_EVENT_ARG;

        const phys_addr_t addr = ((phys_addr_t)argv + sizeof(phys_addr_t) * i);
        const char *argp = NULL;
        bpf_core_read_user(&argp, sizeof(argp), (const void*)addr);
        if (!argp || (bpf_core_read_user_str(ev->data, sizeof(ev->data), argp) <= 0))
            break;
  
        bpf_ringbuf_submit(ev, 0);
    }

    return 0;
}

SEC("tracepoint/syscalls/sys_enter_execve")
int sys_enter_execve(struct trace_event_raw_sys_enter *ctx)
{
    struct task_struct *task = (struct task_struct*)bpf_get_current_task();

    pid_t pid = issue_process_start(task);
    if (pid == (pid_t)-1)
        return 0;

    issue_process_filename(ctx, pid);
    issue_process_args(ctx, pid);
    
    return 0;
}

SEC("tracepoint/syscalls/sys_exit_execve")
int sys_exit_execve(struct trace_event_raw_sys_exit *ctx)
{
    struct task_struct *task = (struct task_struct*)bpf_get_current_task();

    struct process_event_retval_t *ev = bpf_ringbuf_reserve(&g_ringbuf, sizeof(struct process_event_retval_t), 0);
    if (!ev)
        return 0;

    ev->header.pid = (pid_t)(bpf_get_current_pid_tgid() >> 32);
    ev->header.type = PROCESS_EVENT_RETVAL;
    
    ev->retval = BPF_CORE_READ(ctx, ret);

    bpf_ringbuf_submit(ev, 0);

    return 0;
}