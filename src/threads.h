// Author: Holodome
// Date: 25.09.2021 
// File: pkby/src/threads.h
// Version: 0
//
// Defines platform-agnostic API to access and manipulate threads.
#include "general.h"
#if defined(_WIN32)
#define THREAD_PROC_SIGN(_name) unsigned long _name(void *param)
#elif defined(_POSIX_VERSION)
#defien THREAD_PROC_SIGN(_name) void _name(void *param)
#else 
#error Unsupported platform
#endif 

typedef THREAD_PROC_SIGN(ThreadProc);

typedef struct {
    u64 value;
} ThreadID;

ThreadID create_thread(ThreadProc *proc, void *param);
void exit_thread(void);

i64 atomic_add64(volatile i64 *value, i64 addend);