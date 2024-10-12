#include "../lib/hw.h"

#include "../h/console.h"
#include "../h/kernel.h"
#include "../h/mem.h"
#include "../h/scheduler.h"
#include "../h/semaphore.h"
#include "../h/thread.h"

extern void irq_wrap();
extern void userMain();

thread_t kernel_main;
thread_t user_main;
u64 system_stack;

const u64 IRQ_TIMER = (1ULL << 63) | 0x1;
const u64 IRQ_HW = (1ULL << 63) | 0x9;
const u64 IRQ_ILLEGAL_OP = 0x2;
const u64 IRQ_ILLEGAL_READ = 0x5;
const u64 IRQ_ILLEGAL_WRITE = 0x7;
const u64 IRQ_ECALL_USER = 0x8;
const u64 IRQ_ECALL_KERNEL = 0x9;

enum SYSCALL_CODE
{
    SYSCALL_MEM_ALLOC = 0x1,
    SYSCALL_MEM_FREE,
    SYSCALL_THREAD_CREATE = 0x11,
    SYSCALL_THREAD_EXIT,
    SYSCALL_THREAD_DISPATCH,
    SYSCALL_SEM_OPEN = 0x21,
    SYSCALL_SEM_CLOSE,
    SYSCALL_SEM_WAIT,
    SYSCALL_SEM_SIGNAL,
    SYSCALL_SEM_TIMED_WAIT,
    SYSCALL_TIMED_SLEEP = 0x31,
    SYSCALL_GETC = 0x41,
    SYSCALL_PUTC,

    SYSCALL_KERNEL_DISPATCH = 0x91
};

void get_system_stack()
{
    ASM("move t0, %[system_stack]" : : [system_stack] "r" (system_stack));
}

void irq_handler()
{
    u64 scause;
    ASM("csrr %[scause], scause" : [scause] "=r" (scause));

    switch(scause)
    {
        case IRQ_TIMER:
        {
            __debug_str("Timer interrupt\n");

            __scheduler_tick();

            thread_t thread_current = __scheduler_current();
            thread_current->time_left--;

            if(thread_current->time_left == 0)
            {
                __scheduler_push(thread_current);

                thread_t thread_next = __scheduler_next();
                yield(thread_current, thread_next);

                __debug_str("Timer context switch\n");
                return;
            }

            break;
        }
        case IRQ_HW:
        {
            // DO NOT PRINT

            i32 plic = plic_claim();

            if(plic == 10)
            {
                __console_receive();
                __console_send();
            }

            plic_complete(10);

            break;
        }
        case IRQ_ILLEGAL_OP:
        {
            __debug
            (
                __debug_str("Illegal operation\n");

                u64 sepc;
                u64 stval;

                ASM("csrr %[sepc], sepc" : [sepc] "=r" (sepc));
                ASM("csrr %[stval], stval" : [stval] "=r" (stval));

                __debug_mem("sepc", sepc);
                __debug_mem("stval", stval);
            );

            __panic("Illegal operation\n");
            break;
        }
        case IRQ_ILLEGAL_READ:
        {
            __debug
            (
                context_t context;
                ASM("csrr %[ctx], sscratch" : [ctx] "=r" (context));

                __debug_mem("a0", context->a0);
                __debug_str("Illegal read\n");

                u64 sepc;
                u64 stval;

                ASM("csrr %[sepc], sepc" : [sepc] "=r" (sepc));
                ASM("csrr %[stval], stval" : [stval] "=r" (stval));

                __debug_mem("sepc", sepc);
                __debug_mem("stval", stval);
            );

            __panic("Illegal read\n");
            break;
        }
        case IRQ_ILLEGAL_WRITE:
        {
            __debug
            (
                __debug_str("Illegal write\n");

                u64 sepc;
                u64 stval;

                ASM("csrr %[sepc], sepc" : [sepc] "=r" (sepc));
                ASM("csrr %[stval], stval" : [stval] "=r" (stval));

                __debug_mem("sepc", sepc);
                __debug_mem("stval", stval);
            );

            __panic("Illegal write\n");
            break;
        }
        case IRQ_ECALL_USER:
        {
            context_t context;
            ASM("csrr %[ctx], sscratch" : [ctx] "=r" (context));

            ASM("csrr t0, sepc");
            ASM("addi t0, t0, 4");
            ASM("csrw sepc, t0");

            u64 syscall_code = context->a0;

            switch(syscall_code)
            {
                case SYSCALL_MEM_ALLOC:
                {
                    size_t nblocks = context->a1;

                    void *result = __mem_alloc(nblocks);
                    context->a0 = (u64)result;

                    __debug_mem("Blocks allocated", nblocks);
                    __debug_mem("Bytes allocated", nblocks * MEM_BLOCK_SIZE);
                    __debug_mem("Allocated mem start", (u64)result);

                    break;
                }
                case SYSCALL_GETC:
                {
                    // Context switch inside
                    if(__getc())
                    {
                        __debug_str("Blocked to wait for input\n");
                        return;
                    }

                    break;
                }
                case SYSCALL_PUTC:
                {
                    if(__putc(context->a1))
                    {
                        u64 sepc;

                        ASM("csrr t0, sepc");
                        ASM("addi t0, t0, -4");
                        ASM("csrw sepc, t0");

                        ASM("move %[sepc], t0" : [sepc] "=r" (sepc));

                        thread_t thread_current = __scheduler_current();
                        thread_current->sepc = sepc - 4;

                        __scheduler_push(thread_current);

                        thread_t thread_next = __scheduler_next();
                        yield(thread_current, thread_next);

                        __debug
                        (
                            u64 stval;

                            ASM("csrr %[sepc], sepc" : [sepc] "=r" (sepc));
                            ASM("csrr %[stval], stval" : [stval] "=r" (stval));

                            __debug_mem("sepc", sepc);
                            __debug_mem("stval", stval);

                            __debug_str("Receive buffer empty, thread dispatched\n");
                            __debug_str("Send buffer full, thread dispatched\n");
                        );

                        return;
                    }

                    __debug_mem("a0", context->a0);
                    __debug_mem("char", context->a1);

                    break;
                }
                case SYSCALL_THREAD_CREATE:
                {
                    u64 thread_new = context->a1;
                    u64 start_f = context->a2;
                    u64 arg = context->a3;
                    u64 stack_space = context->a4;

                    __debug_mem("new thread handle", thread_new);
                    __debug_mem("start f", start_f);
                    __debug_mem("arg", arg);
                    __debug_mem("stack space", stack_space);

                    i32 result = __thread_create((thread_t *)thread_new, (void *)start_f, (void *)arg, (void *)stack_space, EXEC_MODE_USER);
                    context->a0 = result;

                    __debug_str("Created thread\n");

                    break;
                }
                case SYSCALL_THREAD_EXIT:
                {
                    __debug_str("syscall thread exit\n");

                    __debug
                    (
                        thread_t thread_current = __scheduler_current();
                        __debug_mem("current sp", thread_current->sp);
                        __debug_mem("current bp", thread_current->bp);
                    );

                    __thread_exit();

                    __debug
                    (
                        thread_current = __scheduler_current();
                        __debug_mem("Thread", (u64)thread_current);
                        __debug_mem("sepc", (u64)(thread_current->sepc));
                        __debug_mem("sstatus", (u64)thread_current->sstatus);
                        __debug_mem("sp", (u64)thread_current->sp);
                    );

                    return;
                }
                case SYSCALL_THREAD_DISPATCH:
                {
                    thread_t thread_current = __scheduler_current();
                    __scheduler_push(thread_current);

                    thread_t thread_next = __scheduler_next();
                    yield(thread_current, thread_next);

                    __debug_str("Dispatched thread\n");
                    return;
                }
                case SYSCALL_SEM_OPEN:
                {
                    u64 semaphore = context->a1;
                    u64 init = context->a2;

                    __debug_mem("new semaphore handle", semaphore);
                    __debug_mem("initial value", init);

                    i32 res = __sem_open((sem_t *)semaphore, (u32)init);
                    context->a0 = res;

                    __debug_str("Created semaphore\n");

                    break;
                }
                case SYSCALL_SEM_CLOSE:
                {
                    u64 semaphore = context->a1;

                    __debug_mem("Closing semaphore", semaphore);

                    i32 res = __sem_close((sem_t)semaphore);
                    context->a0 = res;

                    __debug_str("Closed semaphore\n");

                    break;
                }
                case SYSCALL_SEM_WAIT:
                {
                    u64 semaphore = context->a1;

                    __debug_mem("Waiting on semaphore", semaphore);

                    // context switch inside
                    if(__sem_wait((sem_t)semaphore))
                    {
                        __debug_str("Blocked by semaphore\n");

                        return;
                    }

                    context->a0 = 0;

                    __debug_str("Waited on semaphore\n");

                    break;
                }
                case SYSCALL_SEM_SIGNAL:
                {
                    u64 semaphore = context->a1;

                    __debug_mem("Signaling semaphore", semaphore);

                    i32 res = __sem_signal((sem_t)semaphore);
                    context->a0 = res;

                    __debug_str("Signaled semaphore\n");

                    break;
                }
                case SYSCALL_SEM_TIMED_WAIT:
                {
                    u64 semaphore = context->a1;
                    u64 time = context->a2;

                    __debug_mem("Timed waiting on semaphore", semaphore);
                    __debug_mem("Time left", time);

                    // context switch inside
                    if(__sem_timed_wait((sem_t)semaphore, (time_t)time))
                    {
                        __debug_str("Blocked by semaphore\n");

                        return;
                    }

                    context->a0 = 0;

                    __debug_str("Thread set to timed wait");

                    break;
                }
                case SYSCALL_TIMED_SLEEP:
                {
                    u64 time = context->a1;
                    context->a0 = 0;

                    if(time == 0ULL)
                        break;

                    thread_t thread_current = __scheduler_current();
                    __scheduler_timeout(thread_current, (time_t)time, (sem_t)0ULL);

                    thread_t thread_next = __scheduler_next();
                    yield(thread_current, thread_next);

                    __debug_mem("Thread sleep", (u64)thread_current);

                    return;
                }
            }

            __debug
            (
                __debug_mem("User mode syscall", syscall_code);

                u64 sepc;
                u64 stval;

                ASM("csrr %[sepc], sepc" : [sepc] "=r" (sepc));
                ASM("csrr %[stval], stval" : [stval] "=r" (stval));

                __debug_mem("sepc", sepc);
                __debug_mem("stval", stval);
            );

            break;
        }
        case IRQ_ECALL_KERNEL:
        {
            context_t context;
            ASM("csrr %[ctx], sscratch" : [ctx] "=r" (context));

            ASM("csrr t0, sepc");
            ASM("addi t0, t0, 4");
            ASM("csrw sepc, t0");

            u64 syscall_code = context->a0;

            switch(syscall_code)
            {
                case SYSCALL_KERNEL_DISPATCH:
                {
                    __debug_str("Kernel mode syscall\n");

                    __debug_mem("kernel main", (u64)kernel_main);
                    __debug_mem("kernel main sp", (u64)&(kernel_main->sp));

                    thread_t thread_current = __scheduler_current();
                    thread_t thread_next = __scheduler_next();
                    yield(thread_current, thread_next);

                    __debug_mem("kernel main", (u64)kernel_main);
                    __debug_mem("kernel main sp", thread_current->sp);

                    __debug_str("Yielded to userMain\n");
                    __debug_mem("thread wrapper location fron kernel syscall", user_main->sepc);

                    __debug_mem("new thread bp after yield", thread_next->bp);
                    __debug_mem("new thread sp after yield", thread_next->sp);

                    u64 new_sp = thread_next->sp;
                    ASM("csrw sscratch, %[new_sp]" : : [new_sp] "r" (new_sp));

                    break;
                }
            }

            __debug_str("Completed kernel mode syscall\n");

            break;
        }
    }

    return;
}

int main()
{
    ASM("li t0, 0x2");
    ASM("csrc sstatus, t0");
    ASM("csrw stvec, %[irq_wrap]" : : [irq_wrap] "r" (irq_wrap));

    __mem_init();
    __console_init();
    
    __debug_str("bleh\n");

    __debug_mem("HEAP_START_ADDR", (u64)HEAP_START_ADDR);
    __debug_mem("HEAP_END_ADDR", (u64)HEAP_END_ADDR);

    u64 thread_size_in_blocks = (sizeof(*kernel_main) + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
    kernel_main = __mem_alloc(thread_size_in_blocks);

    if(kernel_main == 0ULL)
        __panic("Failed to allocate kernel main thread\n");

    __debug_mem("kernel_main", (u64)kernel_main);
    kernel_main->bp = 0ULL;
    kernel_main->time_left = 1;

    __scheduler_init(kernel_main);

    u64 stack_size_in_blocks = (DEFAULT_STACK_SIZE + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;

    system_stack = (u64)__mem_alloc(stack_size_in_blocks);
    u64 user_stack = (u64)__mem_alloc(stack_size_in_blocks);

    if(system_stack == 0ULL)
        __panic("Failed to allocate system stack\n");

    if(user_stack == 0ULL)
        __panic("Failed to allocate user main stack");

    system_stack += DEFAULT_STACK_SIZE - 8ULL;
    user_stack += DEFAULT_STACK_SIZE - 8ULL;

    __debug_str("Creating user main\n");
    if(__thread_create(&user_main, userMain, 0, (void *)user_stack, EXEC_MODE_USER))
        __panic("Failed to allocate user main thread\n");

    __debug_mem("user_main", (u64)user_main);
    __debug_mem("user_main stack", (u64)user_stack);

    ASM("move a0, %[kernel_main_dispatch]" : : [kernel_main_dispatch] "r" (SYSCALL_KERNEL_DISPATCH));
    ASM("ecall");

    while(__scheduler_user_thread_count())
    {
        u64 sstatus;
        u64 int_enable_sstatus;

        ASM("csrr %[sstatus], sstatus" : [sstatus] "=r" (sstatus));
        int_enable_sstatus = sstatus | (1 << 1);

        // Allow timer interrupt for 1 instruction
        ASM("csrw sstatus, %[int_enable_sstatus]" : : [int_enable_sstatus] "r" (int_enable_sstatus));
        ASM("csrw sstatus, %[sstatus]" : : [sstatus] "r" (sstatus));

        ASM("move a0, %[kernel_main_dispatch]" : : [kernel_main_dispatch] "r" (SYSCALL_KERNEL_DISPATCH));
        ASM("ecall");
    }

    __debug_str("Kernel finished\n");
    __stop();

    return 0;
}
