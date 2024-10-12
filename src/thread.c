#include "../h/scheduler.h"
#include "../h/thread.h"
#include "../h/mem.h"
#include "../h/kernel.h"

enum THREAD_CREATE_ERRORS
{
    THREAD_CREATE_NO_MEMORY = -1,
};

void __thread_wrapper(void(* start_f)(void *), void *arg)
{
    start_f(arg);

    ASM("li a0, 0x12");
    ASM("ecall");
}

int __thread_create(thread_t *handle, void(* start_f)(void *), void *arg, void *stack_space, enum EXEC_MODE mode)
{
    size_t thread_size_in_blocks = (sizeof(struct __thread_t) + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
    thread_t new_thread = __mem_alloc(thread_size_in_blocks);

    if(new_thread == 0ULL)
        return THREAD_CREATE_NO_MEMORY;

    new_thread->sp = (u64)stack_space;
    new_thread->bp = (u64)stack_space - DEFAULT_STACK_SIZE + 8ULL;
    new_thread->time_left = DEFAULT_TIME_SLICE;

    new_thread->sp -= 0x100;
    ((context_t)new_thread->sp)->sp = new_thread->sp;
    ((context_t)new_thread->sp)->a0 = (u64)start_f;
    ((context_t)new_thread->sp)->a1 = (u64)arg;

    __debug_mem("new thread bp", new_thread->bp);
    __debug_mem("new thread sp", new_thread->sp);

    u64 gp;
    u64 tp;

    ASM("move %[gp], x3" : [gp] "=r" (gp));
    ASM("move %[tp], x4" : [tp] "=r" (tp));

    ((context_t)new_thread->sp)->gp = gp;
    ((context_t)new_thread->sp)->tp = tp;

    u64 new_sstatus;

    if(mode == EXEC_MODE_USER)
    {
        ASM("csrr %[new_sstatus], sstatus" : [new_sstatus] "=r" (new_sstatus));
        new_sstatus &= ~(1ULL << 8);

        __scheduler_user_thread_increment();
    }
    else
    {
        ASM("csrr %[new_sstatus], sstatus" : [new_sstatus] "=r" (new_sstatus));
        new_sstatus |= (1ULL << 8);
        new_sstatus &= ~(1ULL << 1);
    }

    __debug_mem("Thread wrapper location", (u64)__thread_wrapper - 4ULL);
    __debug_mem("Function body location", (u64)start_f);

    new_thread->sstatus = new_sstatus;
    new_thread->sepc = (u64)__thread_wrapper;

    *handle = new_thread;
    __scheduler_push(new_thread);

    __debug_str("Pushed to scheduler\n");
    __debug(__scheduler_queue_print());

    return 0;
}

void yield(thread_t thread_old, thread_t thread_new)
{
    __debug_mem("old thread", (u64)thread_old);
    __debug_mem("new thread", (u64)thread_new);

    if(thread_old == thread_new)
    {
        thread_t thread_current = thread_old;
        u64 sp;

        ASM("csrr %[sp], sscratch" : [sp] "=r" (sp));

        thread_current->sp = sp;

        return;
    }

    u64 temp_sstatus = thread_new->sstatus;
    u64 temp_sepc = thread_new->sepc;

    __debug_mem("new sstatus", temp_sstatus);
    __debug_mem("new sepc", temp_sepc);

    ASM("csrrw %[old_sstatus], sstatus, %[new_sstatus]" : [old_sstatus] "=r" (temp_sstatus) : [new_sstatus] "r" (temp_sstatus));
    ASM("csrrw %[old_sepc], sepc, %[new_sepc]" : [old_sepc] "=r" (temp_sepc) : [new_sepc] "r" (temp_sepc));

    __debug_mem("old sstatus", temp_sstatus);
    __debug_mem("old sepc", temp_sepc);

    thread_old->sstatus = temp_sstatus;
    thread_old->sepc = temp_sepc;

    __debug_mem("thread_old sp", thread_old->sp);
    __debug_mem("thread_new sp", thread_new->sp);

    u64 old_sp;

    ASM("csrr %[old_sp], sscratch" : [old_sp] "=r" (old_sp));

    thread_old->sp = old_sp;

    u64 new_sp = thread_new->sp;
    ASM("csrw sscratch, %[new_sp]" : : [new_sp] "r" (new_sp));

    __debug_mem("old sp", thread_old->sp);
    __debug_mem("new sp", thread_new->sp);

    return;
}

void __thread_delete(thread_t thread)
{
    __debug_mem("bp", thread->bp);
    __debug_mem("sp", thread->sp);

    if(thread->sp < thread->bp)
        __panic("Stack overflow\n");

    if(__mem_free((void *)thread->bp))
        __panic("Failed to free thread stack, memory corruption\n");

    if(__mem_free((void *)thread))
       __panic("Failed to free thread, memory corruption\n");

    return;
}

void __thread_exit()
{
    u64 sstatus;
    ASM("csrr %[sstatus], sstatus" : [sstatus] "=r" (sstatus));

    if(!(sstatus & (1ULL << 8)))
        __scheduler_user_thread_decrement();

    __debug_str("Exited thread\n");
    thread_t thread_current = __scheduler_current();
    thread_t thread_next = __scheduler_next();

    yield(thread_current, thread_next);
    __thread_delete(thread_current);

    return;
}
