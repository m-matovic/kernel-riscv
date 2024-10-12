#ifndef THREAD_HEADER
#define THREAD_HEADER

#include "../lib/hw.h"
#include "../h/kernel.h"
#include "../h/list.h"

enum EXEC_MODE
{
    EXEC_MODE_USER,
    EXEC_MODE_KERNEL
};

struct __thread_t
{
    u64 sp;
    u64 bp;
    u64 sstatus;
    u64 sepc;

    time_t time_left;
    list_t list_node;
};

typedef struct __thread_t * thread_t;

void __thread_wrapper(void(* start_f)(void *), void *arg);
int __thread_create(thread_t *handle, void(* start_f)(void *), void *arg, void *stack_space, enum EXEC_MODE mode);
void __thread_delete(thread_t thread);
void __thread_exit();
void __thread_dispatch();
void yield(thread_t thread_old, thread_t thread_new);

#endif //THREAD_HEADER
