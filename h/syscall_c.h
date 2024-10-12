#ifdef __cplusplus
extern "C" {
#endif

#ifndef SYSCALL_C_HEADER
#define SYSCALL_C_HEADER

#include "../lib/hw.h"
#include "../h/kernel.h"
#include "../h/thread.h"
#include "../h/semaphore.h"

void putc(char c);
char getc();
void *mem_alloc(size_t nbytes);
int mem_free(void *ptr);
int thread_create(thread_t *handle, void(* start_f)(void *), void *arg);
int thread_exit();
void thread_dispatch();

int sem_open(sem_t *handle, unsigned int cnt);
int sem_close(sem_t handle);
int sem_wait(sem_t handle);
int sem_signal(sem_t handle);
int sem_timed_wait(sem_t handle, time_t timeout);
int sem_trywait(sem_t handle);

int time_sleep(time_t time);

#endif //SYSCALL_C_HEADER

#ifdef __cplusplus
}
#endif
