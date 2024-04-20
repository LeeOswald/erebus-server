#include "vmlinux.h"
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

#include "execve.bpf.h"

#define PATH_DEPTH_MAX 50
#define MAX_ARGS 150

char _license[] SEC("license") = "GPL v2";

struct {
	__uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
	__uint(key_size, sizeof(__u32));
	__uint(value_size, sizeof(__u32));
} events SEC(".maps");

__attribute__((always_inline)) void capture_numeric_fields(struct task_struct *task, struct process_info_t *process_info)
{
    process_info->pid = (pid_t)(bpf_get_current_pid_tgid() >> 32);
    process_info->ppid = (pid_t)BPF_CORE_READ(task, real_parent, tgid);
    process_info->uid = bpf_get_current_uid_gid() & 0xffffffff;
    process_info->sid = (__u32)BPF_CORE_READ(task, sessionid);
    process_info->start_time = (__u64)BPF_CORE_READ(task, se.exec_start);
}

__attribute__((always_inline)) void capture_comm(struct trace_event_raw_sys_enter *ctx, struct process_info_t *process_info)
{
    process_info->type = EVENT_COMM;

    bpf_get_current_comm(&process_info->data, sizeof(process_info->data));
    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, process_info, sizeof(struct process_info_t));
}

__attribute__((always_inline)) unsigned long get_inode_ino(struct inode *inode)
{
    unsigned long ino;
    BPF_CORE_READ_INTO(&ino, inode, i_ino);
    return ino;
}

__attribute__((always_inline)) unsigned long get_dentry_ino(struct dentry *dentry)
{
    struct inode *d_inode;
    BPF_CORE_READ_INTO(&d_inode, dentry, d_inode);
    return get_inode_ino(d_inode);
}

__attribute__((always_inline)) void capture_cwd(struct trace_event_raw_sys_enter *ctx, struct dentry *dentry, struct process_info_t *process_info)
{
    struct qstr qstr;
    struct dentry *d_parent;

    process_info->type = EVENT_CWD;

#pragma clang loop unroll(full)
    for (int i = 0; i < PATH_DEPTH_MAX; ++i)
    {
        BPF_CORE_READ_INTO(&qstr, dentry, d_name);
        bpf_core_read_str(process_info->data, sizeof(process_info->data), qstr.name);

        BPF_CORE_READ_INTO(&d_parent, dentry, d_parent);
        if (get_dentry_ino(dentry) == get_dentry_ino(d_parent))
        {
            break;
        }
        bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, process_info, sizeof(struct process_info_t));
        dentry = d_parent;
    }
}

__attribute__((always_inline)) void capture_argv(struct trace_event_raw_sys_enter *ctx, const char **argv, struct process_info_t *process_info)
{
    const char *argp;

    process_info->type = EVENT_ARG;

#pragma clang loop unroll(full)
	for (int i = 0; i < MAX_ARGS; ++i)
    {
        const phys_addr_t addr = ((phys_addr_t)argv + sizeof(phys_addr_t) * i);
        bpf_core_read_user(&argp, sizeof(argp), (const void*)addr);
        if (!argp || (bpf_core_read_user_str(process_info->data, sizeof(process_info->data), argp) <= 0))
        {
            break;
        }
        bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, process_info, sizeof(struct process_info_t));
    }
}

__attribute__((always_inline)) void capture_filename(struct trace_event_raw_sys_enter *ctx, const char *filename, struct process_info_t *process_info)
{
    process_info->type = EVENT_FILENAME;

    if (bpf_core_read_user_str(process_info->data, sizeof(process_info->data), filename) <= 0)
    {
        process_info->data[0] = '\0';
    }
    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, process_info, sizeof(struct process_info_t));
}

__attribute__((always_inline)) void capture_retval(struct trace_event_raw_sys_exit *ctx, struct process_info_t *process_info)
{
    process_info->type = EVENT_RETVAL;

    process_info->retval = BPF_CORE_READ(ctx, ret);
    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, process_info, sizeof(struct process_info_t));
}

SEC("tracepoint/syscalls/sys_enter_execve")
int sys_enter_execve(struct trace_event_raw_sys_enter *ctx)
{
    struct process_info_t process_info;
    __builtin_memset(&process_info, 0, sizeof(process_info));

    struct task_struct *task = (struct task_struct*)bpf_get_current_task();

    capture_numeric_fields(task, &process_info);
    capture_argv(ctx, (const char**)(BPF_CORE_READ(ctx, args[1])), &process_info);
    capture_filename(ctx, (const char*)(BPF_CORE_READ(ctx, args[0])), &process_info);

    return 0;
}

SEC("tracepoint/syscalls/sys_exit_execve")
int sys_exit_execve(struct trace_event_raw_sys_exit *ctx)
{
    struct process_info_t process_info;
    __builtin_memset(&process_info, 0, sizeof(process_info));

    struct task_struct *task = (struct task_struct*)bpf_get_current_task();

    capture_numeric_fields(task, &process_info);
    capture_retval(ctx, &process_info);

    return 0;
}

