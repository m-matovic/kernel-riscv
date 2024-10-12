#include "../h/syscall_c.h"
#include "../lib/console.h"

enum THREAD_CREATE_ERRORS
{
    THREAD_CREATE_NO_MEMORY = -1,
};

void putc(char c)
{
    u64 a1;
    __asm__ __volatile__ ("move %[a1], a1" : [a1] "=r" (a1));

    __asm__ __volatile__ ("move a1, a0");
    __asm__ __volatile__ ("li a0, 0x42");
    __asm__ __volatile__ ("ecall");

    __asm__ __volatile__ ("move a1, %[a1]" : : [a1] "r" (a1));

    return;
}

char getc()
{
    __asm__ __volatile__ ("li a0, 0x41");
    __asm__ __volatile__ ("ecall");

    char c;
    __asm__ __volatile__ ("move %[character], a0" : [character] "=r" (c));

    return c;
}

void *mem_alloc(size_t nbytes)
{
    size_t nblocks = (nbytes + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
    void *res;

    u64 a1;
    __asm__ __volatile__ ("move %[a1], a1" : [a1] "=r" (a1));

    __asm__ __volatile__ ("move a1, %[nblocks]" : : [nblocks] "r" (nblocks));
    __asm__ __volatile__ ("li a0, 0x1");
    __asm__ __volatile__ ("ecall");

    __asm__ __volatile__ ("move a1, %[a1]" : : [a1] "r" (a1));
    __asm__ __volatile__ ("move %[res], a0" : [res] "=r" (res));
    return res;
}

int mem_free(void *ptr)
{
    int res;
    u64 a1;
    __asm__ __volatile__ ("move %[a1], a1" : [a1] "=r" (a1));

    __asm__ __volatile__ ("move a1, %[ptr]" : : [ptr] "r" (ptr));
    __asm__ __volatile__ ("li a0, 0x2");
    __asm__ __volatile__ ("ecall");

    __asm__ __volatile__ ("move a1, %[a1]" : : [a1] "r" (a1));
    __asm__ __volatile__ ("move %[res], a0" : [res] "=r" (res));
    return res;
}

int thread_create(thread_t *handle, void(* start_f)(void *), void *arg)
{
    // Offer sacrifice to the machine spirit

    u64 a1, a2, a3, a4;
    __asm__ __volatile__ ("move %[a1], a1" : [a1] "=r" (a1));
    __asm__ __volatile__ ("move %[a2], a2" : [a2] "=r" (a2));
    __asm__ __volatile__ ("move %[a3], a3" : [a3] "=r" (a3));
    __asm__ __volatile__ ("move %[a4], a4" : [a4] "=r" (a4));

    __asm__ __volatile__ ("move a4, a2");
    __asm__ __volatile__ ("move a3, a1");
    __asm__ __volatile__ ("move a2, a0");

    u64 new_stack = (u64)mem_alloc(DEFAULT_STACK_SIZE);

    if(new_stack == 0ULL)
        return THREAD_CREATE_NO_MEMORY;

    new_stack += DEFAULT_STACK_SIZE - 8;

    __asm__ __volatile__ ("move a1, a2");
    __asm__ __volatile__ ("move a2, a3");
    __asm__ __volatile__ ("move a3, a4");
    __asm__ __volatile__ ("move a4, %[new_stack]" : : [new_stack] "r" (new_stack));

    __asm__ __volatile__ ("li a0, 0x11");
    __asm__ __volatile__ ("ecall");

    __asm__ __volatile__ ("move a1, %[a1]" : : [a1] "r" (a1));
    __asm__ __volatile__ ("move a2, %[a2]" : : [a2] "r" (a2));
    __asm__ __volatile__ ("move a3, %[a3]" : : [a3] "r" (a3));
    __asm__ __volatile__ ("move a4, %[a4]" : : [a4] "r" (a4));

    int result;
    __asm__ __volatile__ ("move %[result], a0" : [result] "=r" (result));

    if(result)
        mem_free((void *)(new_stack - DEFAULT_STACK_SIZE + 8));

    return result;
}

int thread_exit()
{
    __asm__ __volatile__ ("li a0, 0x12");
    __asm__ __volatile__ ("ecall");

    return 0;
}

void thread_dispatch()
{
    __asm__ __volatile__ ("li a0, 0x13");
    __asm__ __volatile__ ("ecall");
}


int sem_open(sem_t *handle, unsigned int cnt)
{
    u64 a1, a2;
    __asm__ __volatile__ ("move %[a1], a1" : [a1] "=r" (a1));
    __asm__ __volatile__ ("move %[a2], a2" : [a2] "=r" (a2));

    __asm__ __volatile__ ("move a2, a1");
    __asm__ __volatile__ ("move a1, a0");

    __asm__ __volatile__ ("li a0, 0x21");
    __asm__ __volatile__ ("ecall");

    __asm__ __volatile__ ("move a1, %[a1]" : : [a1] "r" (a1));
    __asm__ __volatile__ ("move a2, %[a2]" : : [a2] "r" (a2));

    i32 res;
    __asm__ __volatile__ ("move %[res], a0" : [res] "=r" (res));

    return res;
}

int sem_close(sem_t handle)
{
    u64 a1;
    __asm__ __volatile__ ("move %[a1], a1" : [a1] "=r" (a1));

    __asm__ __volatile__ ("move a1, a0");

    __asm__ __volatile__ ("li a0, 0x22");
    __asm__ __volatile__ ("ecall");

    __asm__ __volatile__ ("move a1, %[a1]" : : [a1] "r" (a1));

    i32 res;
    __asm__ __volatile__ ("move %[res], a0" : [res] "=r" (res));

    return res;
}

int sem_wait(sem_t handle)
{
    u64 a1;
    __asm__ __volatile__ ("move %[a1], a1" : [a1] "=r" (a1));

    __asm__ __volatile__ ("move a1, a0");

    __asm__ __volatile__ ("li a0, 0x23");
    __asm__ __volatile__ ("ecall");

    __asm__ __volatile__ ("move a1, %[a1]" : : [a1] "r" (a1));

    i32 res;
    __asm__ __volatile__ ("move %[res], a0" : [res] "=r" (res));

    return res;
}

int sem_signal(sem_t handle)
{
    u64 a1;
    __asm__ __volatile__ ("move %[a1], a1" : [a1] "=r" (a1));

    __asm__ __volatile__ ("move a1, a0");

    __asm__ __volatile__ ("li a0, 0x24");
    __asm__ __volatile__ ("ecall");

    __asm__ __volatile__ ("move a1, %[a1]" : : [a1] "r" (a1));

    i32 res;
    __asm__ __volatile__ ("move %[res], a0" : [res] "=r" (res));

    return res;
}

int sem_timed_wait(sem_t handle, time_t timeout)
{
    u64 a1, a2;
    __asm__ __volatile__ ("move %[a1], a1" : [a1] "=r" (a1));
    __asm__ __volatile__ ("move %[a2], a2" : [a2] "=r" (a2));

    __asm__ __volatile__ ("move a2, a1");
    __asm__ __volatile__ ("move a1, a0");

    __asm__ __volatile__ ("li a0, 0x25");
    __asm__ __volatile__ ("ecall");

    __asm__ __volatile__ ("move a1, %[a1]" : : [a1] "r" (a1));
    __asm__ __volatile__ ("move a2, %[a2]" : : [a2] "r" (a2));

    i32 res;
    __asm__ __volatile__ ("move %[res], a0" : [res] "=r" (res));

    return res;
}

int sem_trywait(sem_t handle)
{
    u64 a1;
    __asm__ __volatile__ ("move %[a1], a1" : [a1] "=r" (a1));

    __asm__ __volatile__ ("move a1, a0");

    __asm__ __volatile__ ("li a0, 0x26");
    __asm__ __volatile__ ("ecall");

    __asm__ __volatile__ ("move a1, %[a1]" : : [a1] "r" (a1));

    i32 res;
    __asm__ __volatile__ ("move %[res], a0" : [res] "=r" (res));

    return res;
}

int time_sleep(time_t time)
{
    u64 a1;
    __asm__ __volatile__ ("move %[a1], a1" : [a1] "=r" (a1));

    __asm__ __volatile__ ("move a1, a0");

    __asm__ __volatile__ ("li a0, 0x31");
    __asm__ __volatile__ ("ecall");

    __asm__ __volatile__ ("move a1, %[a1]" : : [a1] "r" (a1));

    i32 res;
    __asm__ __volatile__ ("move %[res], a0" : [res] "=r" (res));

    return res;
}
