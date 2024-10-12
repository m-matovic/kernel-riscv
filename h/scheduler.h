#ifndef SCHEDULER_HEADER
#define SCHEDULER_HEADER

#include "../h/mem.h"
#include "../h/thread.h"
#include "../h/semaphore.h"
#include "../h/list.h"

struct __sleeping_thread_t
{
    thread_t thread;
    sem_t semaphore;
    time_t delta;
    list_t list_node;
};

struct __scheduler_t
{
    u64 user_thread_count;
    list_t *ready_threads;
    list_t *sleeping_threads;
    thread_t thread_current;
    thread_t kernel_main;
};

typedef struct __scheduler_t * scheduler_t;
typedef struct __sleeping_thread_t * sleeping_thread_t;

void __scheduler_init(thread_t kernel_main);
void __scheduler_push(thread_t new_thread);
thread_t __scheduler_current();
thread_t __scheduler_next();
void __scheduler_user_thread_increment();
void __scheduler_user_thread_decrement();
u64 __scheduler_user_thread_count();
void __scheduler_queue_print();
void __scheduler_blocked_print();

void __scheduler_timeout(thread_t thread, time_t time, sem_t semaphore);
void __scheduler_remove_timeout(thread_t thread);
void __scheduler_tick();

#endif //SCHEDULER_HEADER
