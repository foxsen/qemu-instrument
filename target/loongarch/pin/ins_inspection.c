#include "ins_inspection.h"
#include "../instrument/ins.h"
#include "../instrument/error.h"
#include "../instrument/decoder/la_print.h"

ADDRINT INS_Address(INS ins)
{
    return ins->pc;
}

BOOL INS_IsMemoryRead(INS ins)
{
    return op_is_load(ins->origin_ins->op);
}

BOOL INS_IsMemoryWrite(INS ins)
{
    return op_is_store(ins->origin_ins->op);
}

USIZE INS_MemoryOperandSize(INS ins, UINT32 memoryOp)
{
    /* memoryOp is not used */
    IR2_OPCODE op = ins->origin_ins->op;
    switch (op) {
    case LISA_LD_B:
    case LISA_ST_B:
    case LISA_LD_BU:
    case LISA_VLDREPL_B:
    case LISA_VSTELM_B:
    case LISA_XVLDREPL_B:
    case LISA_XVSTELM_B:
    case LISA_LDX_B:
    case LISA_STX_B:
    case LISA_LDX_BU:
        return 1;
    case LISA_LD_H:
    case LISA_ST_H:
    case LISA_LD_HU:
    case LISA_VLDREPL_H:
    case LISA_VSTELM_H:
    case LISA_XVLDREPL_H:
    case LISA_XVSTELM_H:
    case LISA_LDX_H:
    case LISA_STX_H:
    case LISA_LDX_HU:
        return 2;
    case LISA_LD_W:
    case LISA_ST_W:
    case LISA_LL_W:
    case LISA_SC_W:
    case LISA_LDPTR_W:
    case LISA_STPTR_W:
    case LISA_LD_WU:
    case LISA_FLD_S:
    case LISA_FST_S:
    case LISA_VLDREPL_W:
    case LISA_VSTELM_W:
    case LISA_XVLDREPL_W:
    case LISA_XVSTELM_W:
    case LISA_LDX_W:
    case LISA_STX_W:
    case LISA_LDX_WU:
        return 4;
    case LISA_LD_D:
    case LISA_ST_D:
    case LISA_LL_D:
    case LISA_SC_D:
    case LISA_LDPTR_D:
    case LISA_STPTR_D:
    case LISA_FLD_D:
    case LISA_FST_D:
    case LISA_VLDREPL_D:
    case LISA_VSTELM_D:
    case LISA_XVLDREPL_D:
    case LISA_XVSTELM_D:
    case LISA_LDX_D:
    case LISA_STX_D:
        return 8;
    case LISA_VLD:
    case LISA_VST:
        return 16;
    case LISA_XVLD:
    case LISA_XVST:
        return 32;
    case LISA_PRELD:
    /* case LISA_LDL_W: */
    /* case LISA_LDR_W: */
    /* case LISA_LDL_D: */
    /* case LISA_LDR_D: */
    /* case LISA_STL_W: */
    /* case LISA_STR_W: */
    /* case LISA_STL_D: */
    /* case LISA_STR_D: */
    default:
        print_ins(ins->origin_ins);
        lsassertm(0, "unhandled ins op\n");
    }
    return -1;
}


BOOL INS_HasFallThrough(INS ins)
{
    /* TODO: does syscall & break have fallthrough? */
    IR2_OPCODE op = ins->origin_ins->op;
    if (op_is_branch(op) && !op_is_condition_branch(op))
        return false;
    return true;
}

/* TRACE */
BBL TRACE_BblHead(TRACE trace)
{
    return trace->bbl_head;
}

BBL TRACE_BblTail(TRACE trace)
{
    return trace->bbl_tail;
}

UINT32 TRACE_NumBbl(TRACE trace)
{
    return trace->nr_bbl;
}

UINT32 TRACE_NumIns(TRACE trace)
{
    return trace->nr_ins;
}


/* BBL */
UINT32 BBL_NumIns(BBL bbl)
{
    return bbl->nr_ins;
}

INS BBL_InsHead(BBL x)
{
    return x->ins_head;
}

INS BBL_InsTail(BBL x)
{
    return x->ins_tail;
}

BBL BBL_Next(BBL x)
{
    return x->next;
}

BBL BBL_Prev(BBL x)
{
    return x->prev;
}

BOOL BBL_Valid(BBL x)
{
    return (x != NULL);
}

BOOL INS_Valid(INS x)
{
    return (x != NULL);
}



static const char *syscall_nr_name[] = {
    "io_setup",                   // 0
    "io_destroy",                 // 1
    "io_submit",                  // 2
    "io_cancel",                  // 3
    "io_getevents",               // 4
    "setxattr",                   // 5
    "lsetxattr",                  // 6
    "fsetxattr",                  // 7
    "getxattr",                   // 8
    "lgetxattr",                  // 9
    "fgetxattr",                  // 10
    "listxattr",                  // 11
    "llistxattr",                 // 12
    "flistxattr",                 // 13
    "removexattr",                // 14
    "lremovexattr",               // 15
    "fremovexattr",               // 16
    "getcwd",                     // 17
    "lookup_dcookie",             // 18
    "eventfd2",                   // 19
    "epoll_create1",              // 20
    "epoll_ctl",                  // 21
    "epoll_pwait",                // 22
    "dup",                        // 23
    "dup3",                       // 24
    "264_fcntl",                  // 25
    "inotify_init1",              // 26
    "inotify_add_watch",          // 27
    "inotify_rm_watch",           // 28
    "ioctl",                      // 29
    "ioprio_set",                 // 30
    "ioprio_get",                 // 31
    "flock",                      // 32
    "mknodat",                    // 33
    "mkdirat",                    // 34
    "unlinkat",                   // 35
    "symlinkat",                  // 36
    "linkat",                     // 37
    "renameat",                   // 38
    "umount2",                    // 39
    "mount",                      // 40
    "pivot_root",                 // 41
    "nfsservctl",                 // 42
    "264_statfs",                 // 43
    "264_fstatfs",                // 44
    "264_truncate",               // 45
    "264_ftruncate",              // 46
    "fallocate",                  // 47
    "faccessat",                  // 48
    "chdir",                      // 49
    "fchdir",                     // 50
    "chroot",                     // 51
    "fchmod",                     // 52
    "fchmodat",                   // 53
    "fchownat",                   // 54
    "fchown",                     // 55
    "openat",                     // 56
    "close",                      // 57
    "vhangup",                    // 58
    "pipe2",                      // 59
    "quotactl",                   // 60
    "getdents64",                 // 61
    "264_lseek",                  // 62
    "read",                       // 63
    "write",                      // 64
    "readv",                      // 65
    "writev",                     // 66
    "pread64",                    // 67
    "pwrite64",                   // 68
    "preadv",                     // 69
    "pwritev",                    // 70
    "264_sendfile",               // 71
    "pselect6",                   // 72
    "ppoll",                      // 73
    "signalfd4",                  // 74
    "vmsplice",                   // 75
    "splice",                     // 76
    "tee",                        // 77
    "readlinkat",                 // 78
    "264_fstatat",                // 79
    "264_fstat",                  // 80
    "sync",                       // 81
    "fsync",                      // 82
    "fdatasync",                  // 83
    /* "sync_file_range2",           // 84 */
    "sync_file_range",            // 84
    "timerfd_create",             // 85
    "timerfd_settime",            // 86
    "timerfd_gettime",            // 87
    "utimensat",                  // 88
    "acct",                       // 89
    "capget",                     // 90
    "capset",                     // 91
    "personality",                // 92
    "exit",                       // 93
    "exit_group",                 // 94
    "waitid",                     // 95
    "set_tid_address",            // 96
    "unshare",                    // 97
    "futex",                      // 98
    "set_robust_list",            // 99
    "get_robust_list",            // 100
    "nanosleep",                  // 101
    "getitimer",                  // 102
    "setitimer",                  // 103
    "kexec_load",                 // 104
    "init_module",                // 105
    "delete_module",              // 106
    "timer_create",               // 107
    "timer_gettime",              // 108
    "timer_getoverrun",           // 109
    "timer_settime",              // 110
    "timer_delete",               // 111
    "clock_settime",              // 112
    "clock_gettime",              // 113
    "clock_getres",               // 114
    "clock_nanosleep",            // 115
    "syslog",                     // 116
    "ptrace",                     // 117
    "sched_setparam",             // 118
    "sched_setscheduler",         // 119
    "sched_getscheduler",         // 120
    "sched_getparam",             // 121
    "sched_setaffinity",          // 122
    "sched_getaffinity",          // 123
    "sched_yield",                // 124
    "sched_get_priority_max",     // 125
    "sched_get_priority_min",     // 126
    "sched_rr_get_interval",      // 127
    "restart_syscall",            // 128
    "kill",                       // 129
    "tkill",                      // 130
    "tgkill",                     // 131
    "sigaltstack",                // 132
    "rt_sigsuspend",              // 133
    "rt_sigaction",               // 134
    "rt_sigprocmask",             // 135
    "rt_sigpending",              // 136
    "rt_sigtimedwait",            // 137
    "rt_sigqueueinfo",            // 138
    "rt_sigreturn",               // 139
    "setpriority",                // 140
    "getpriority",                // 141
    "reboot",                     // 142
    "setregid",                   // 143
    "setgid",                     // 144
    "setreuid",                   // 145
    "setuid",                     // 146
    "setresuid",                  // 147
    "getresuid",                  // 148
    "setresgid",                  // 149
    "getresgid",                  // 150
    "setfsuid",                   // 151
    "setfsgid",                   // 152
    "times",                      // 153
    "setpgid",                    // 154
    "getpgid",                    // 155
    "getsid",                     // 156
    "setsid",                     // 157
    "getgroups",                  // 158
    "setgroups",                  // 159
    "uname",                      // 160
    "sethostname",                // 161
    "setdomainname",              // 162
    "getrlimit",                  // 163
    "setrlimit",                  // 164
    "getrusage",                  // 165
    "umask",                      // 166
    "prctl",                      // 167
    "getcpu",                     // 168
    "gettimeofday",               // 169
    "settimeofday",               // 170
    "adjtimex",                   // 171
    "getpid",                     // 172
    "getppid",                    // 173
    "getuid",                     // 174
    "geteuid",                    // 175
    "getgid",                     // 176
    "getegid",                    // 177
    "gettid",                     // 178
    "sysinfo",                    // 179
    "mq_open",                    // 180
    "mq_unlink",                  // 181
    "mq_timedsend",               // 182
    "mq_timedreceive",            // 183
    "mq_notify",                  // 184
    "mq_getsetattr",              // 185
    "msgget",                     // 186
    "msgctl",                     // 187
    "msgrcv",                     // 188
    "msgsnd",                     // 189
    "semget",                     // 190
    "semctl",                     // 191
    "semtimedop",                 // 192
    "semop",                      // 193
    "shmget",                     // 194
    "shmctl",                     // 195
    "shmat",                      // 196
    "shmdt",                      // 197
    "socket",                     // 198
    "socketpair",                 // 199
    "bind",                       // 200
    "listen",                     // 201
    "accept",                     // 202
    "connect",                    // 203
    "getsockname",                // 204
    "getpeername",                // 205
    "sendto",                     // 206
    "recvfrom",                   // 207
    "setsockopt",                 // 208
    "getsockopt",                 // 209
    "shutdown",                   // 210
    "sendmsg",                    // 211
    "recvmsg",                    // 212
    "readahead",                  // 213
    "brk",                        // 214
    "munmap",                     // 215
    "mremap",                     // 216
    "add_key",                    // 217
    "request_key",                // 218
    "keyctl",                     // 219
    "clone",                      // 220
    "execve",                     // 221
    "264_mmap",                   // 222
    "264_fadvise64",              // 223
    "swapon",                     // 224
    "swapoff",                    // 225
    "mprotect",                   // 226
    "msync",                      // 227
    "mlock",                      // 228
    "munlock",                    // 229
    "mlockall",                   // 230
    "munlockall",                 // 231
    "mincore",                    // 232
    "madvise",                    // 233
    "remap_file_pages",           // 234
    "mbind",                      // 235
    "get_mempolicy",              // 236
    "set_mempolicy",              // 237
    "migrate_pages",              // 238
    "move_pages",                 // 239
    "rt_tgsigqueueinfo",          // 240
    "perf_event_open",            // 241
    "accept4",                    // 242
    "recvmmsg",                   // 243
    "arch_specific_syscall",      // 244
    "undefined",                       // 245
    "undefined",                       // 246
    "undefined",                       // 247
    "undefined",                       // 248
    "undefined",                       // 249
    "undefined",                       // 250
    "undefined",                       // 251
    "undefined",                       // 252
    "undefined",                       // 253
    "undefined",                       // 254
    "undefined",                       // 255
    "undefined",                       // 256
    "undefined",                       // 257
    "undefined",                       // 258
    "undefined",                       // 259
    "wait4",                      // 260
    "prlimit64",                  // 261
    "fanotify_init",              // 262
    "fanotify_mark",              // 263
    "name_to_handle_at",          // 264
    "open_by_handle_at",          // 265
    "clock_adjtime",              // 266
    "syncfs",                     // 267
    "setns",                      // 268
    "sendmmsg",                   // 269
    "process_vm_readv",           // 270
    "process_vm_writev",          // 271
    "kcmp",                       // 272
    "finit_module",               // 273
    "sched_setattr",              // 274
    "sched_getattr",              // 275
    "renameat2",                  // 276
    "seccomp",                    // 277
    "getrandom",                  // 278
    "memfd_create",               // 279
    "bpf",                        // 280
    "execveat",                   // 281
    "userfaultfd",                // 282
    "membarrier",                 // 283
    "mlock2",                     // 284
    "copy_file_range",            // 285
    "preadv2",                    // 286
    "pwritev2",                   // 287
    "pkey_mprotect",              // 288
    "pkey_alloc",                 // 289
    "pkey_free",                  // 290
    "statx",                      // 291
    "io_pgetevents",              // 292
    "rseq",                       // 293
    "syscalls",                   // 294
};

const char* syscall_name(uint64_t nr)
{
    return syscall_nr_name[nr];
}
