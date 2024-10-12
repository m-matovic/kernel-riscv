#ifndef SEMAPHORE_HEADER
#define SEMAPHORE_HEADER

#include "../h/list.h"
#include "../h/kernel.h"
#include "../h/thread.h"
#include "../h/mem.h"

struct __sem_t
{
    i32 val;
    i32 init;
    list_t *waiting_threads;
};

typedef struct __sem_t * sem_t;

int __sem_open(sem_t *handle, u32 init);
int __sem_close(sem_t handle);
int __sem_wait(sem_t handle);
int __sem_unblock_thread(sem_t handle);
int __sem_signal(sem_t handle);
int __sem_timed_wait(sem_t handle, time_t timeout);
int __sem_trywait(sem_t handle);
void __sem_delete(sem_t handle);
void __sem_remove_thread(sem_t handle, thread_t thread);
void __sem_first_print();
void __sem_all_print();

#endif //SEMAPHORE_HEADER
