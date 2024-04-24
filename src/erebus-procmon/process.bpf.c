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


__attribute__((always_inline)) pid_t issue_execve_enter(struct task_struct *task)
{
    struct process_event_execve_enter_t *ev = bpf_ringbuf_reserve(&g_ringbuf, sizeof(struct process_event_execve_enter_t), 0);
    if (!ev)
        return (pid_t)-1;

    pid_t pid = (pid_t)(bpf_get_current_pid_tgid() >> 32);
    ev->header.pid = pid;
    ev->header.type = PROCESS_EVENT_EXECVE_ENTER;

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
    ev->header.type = PROCESS_EVENT_EXECVE_FILENAME;

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
        ev->header.type = PROCESS_EVENT_EXECVE_ARG;

        const phys_addr_t addr = ((phys_addr_t)argv + sizeof(phys_addr_t) * i);
        const char *argp = NULL;
        bpf_core_read_user(&argp, sizeof(argp), (const void*)addr);
        if (!argp || (bpf_core_read_user_str(ev->data, sizeof(ev->data), argp) <= 0))
        {
            bpf_ringbuf_discard(ev, 0);
            break;
        }
  
        bpf_ringbuf_submit(ev, 0);
    }

    return 0;
}

__attribute__((always_inline)) int issue_fork_enter(struct task_struct *task, enum process_event_type type)
{
    struct process_event_fork_enter_t *ev = bpf_ringbuf_reserve(&g_ringbuf, sizeof(struct process_event_fork_enter_t), 0);
    if (!ev)
        return -1;

    ev->header.pid = (pid_t)(bpf_get_current_pid_tgid() >> 32);
    ev->header.type = type;

    bpf_get_current_comm(&ev->comm, sizeof(ev->comm));

    bpf_ringbuf_submit(ev, 0);
    return 0;
}

__attribute__((always_inline)) int generic_sys_exit(struct trace_event_raw_sys_exit *ctx, enum process_event_type type)
{
    struct task_struct *task = (struct task_struct*)bpf_get_current_task();

    struct process_event_retval_t *ev = bpf_ringbuf_reserve(&g_ringbuf, sizeof(struct process_event_retval_t), 0);
    if (!ev)
        return 0;

    ev->header.pid = (pid_t)(bpf_get_current_pid_tgid() >> 32);
    ev->header.type = type;
    
    ev->retval = BPF_CORE_READ(ctx, ret);

    bpf_ringbuf_submit(ev, 0);

    return 0;
}

SEC("tracepoint/syscalls/sys_enter_execve")
int sys_enter_execve(struct trace_event_raw_sys_enter *ctx)
{
    struct task_struct *task = (struct task_struct*)bpf_get_current_task();

    pid_t pid = issue_execve_enter(task);
    if (pid == (pid_t)-1)
        return 0;

    issue_process_filename(ctx, pid);
    issue_process_args(ctx, pid);
    
    return 0;
}

SEC("tracepoint/syscalls/sys_exit_execve")
int sys_exit_execve(struct trace_event_raw_sys_exit *ctx)
{
    return generic_sys_exit(ctx, PROCESS_EVENT_EXECVE_RETVAL);
}

SEC("tracepoint/syscalls/sys_enter_fork")
int sys_enter_fork(struct trace_event_raw_sys_enter *ctx)
{
    struct task_struct *task = (struct task_struct*)bpf_get_current_task();

    issue_fork_enter(task, PROCESS_EVENT_FORK_ENTER);
    
    return 0;
}

SEC("tracepoint/syscalls/sys_exit_fork")
int sys_exit_fork(struct trace_event_raw_sys_exit *ctx)
{
    return generic_sys_exit(ctx, PROCESS_EVENT_FORK_RETVAL);
}

SEC("tracepoint/syscalls/sys_enter_vfork")
int sys_enter_vfork(struct trace_event_raw_sys_enter *ctx)
{
    struct task_struct *task = (struct task_struct*)bpf_get_current_task();

    issue_fork_enter(task, PROCESS_EVENT_VFORK_ENTER);
    
    return 0;
}

SEC("tracepoint/syscalls/sys_exit_vfork")
int sys_exit_vfork(struct trace_event_raw_sys_exit *ctx)
{
    return generic_sys_exit(ctx, PROCESS_EVENT_VFORK_RETVAL);
}

SEC("tp/sched/sched_process_exit")
int sched_process_exit(struct trace_event_raw_sched_process_template *ctx)
{
    struct task_struct *task = (struct task_struct*)bpf_get_current_task();

    u64 id = bpf_get_current_pid_tgid();
    pid_t pid = id >> 32;
    pid_t tid = (u32)id;

    struct process_event_exit_t *ev = bpf_ringbuf_reserve(&g_ringbuf, sizeof(struct process_event_exit_t), 0);
    if (!ev)
        return 0;

    ev->header.pid = pid;
    ev->header.type = PROCESS_EVENT_EXIT;
    
    ev->tid = tid;
    ev->exit_code = (BPF_CORE_READ(task, exit_code) >> 8) & 0xff;
 
    bpf_ringbuf_submit(ev, 0);
 
    return 0;
}