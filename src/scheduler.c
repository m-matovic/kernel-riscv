#include "../h/mem.h"
#include "../h/kernel.h"
#include "../h/scheduler.h"

scheduler_t scheduler;

void __scheduler_init(thread_t kernel_main)
{
    u64 scheduler_size_in_blocks = (sizeof(*scheduler) + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
    scheduler = __mem_alloc(scheduler_size_in_blocks);

    if(scheduler == 0ULL)
        __panic("Failed to allocate scheduler\n");

    scheduler->user_thread_count = 0ULL;
    scheduler->ready_threads = 0ULL;
    scheduler->sleeping_threads = 0ULL;
    scheduler->kernel_main = kernel_main;
    scheduler->thread_current = kernel_main;
}

void __scheduler_push(thread_t new_thread)
{
    __debug_mem("Pushing thread", (u64)new_thread);

    new_thread->time_left = DEFAULT_TIME_SLICE;

    if(scheduler->ready_threads == 0ULL)
    {
        new_thread->list_node.next = &(new_thread->list_node);
        new_thread->list_node.prev = &(new_thread->list_node);
        scheduler->ready_threads = &(new_thread->list_node);

        return;
    }

    list_insert(scheduler->ready_threads, &(new_thread->list_node));

    __debug
    (
        __scheduler_queue_print();
    );

    return;
}

thread_t __scheduler_next()
{
    if(scheduler->ready_threads == 0ULL)
    {
        __debug_str("Next is kernel main\n");
        scheduler->thread_current = scheduler->kernel_main;
        return scheduler->kernel_main;
    }

    if(scheduler->ready_threads->next == scheduler->ready_threads)
    {
        thread_t next_thread = container_of(scheduler->ready_threads, struct __thread_t, list_node);
        scheduler->thread_current = next_thread;
        scheduler->ready_threads = 0ULL;

        return next_thread;
    }

    thread_t next_thread = container_of(scheduler->ready_threads, struct __thread_t, list_node);

    scheduler->ready_threads->prev->next = scheduler->ready_threads->next;
    scheduler->ready_threads->next->prev = scheduler->ready_threads->prev;

    scheduler->ready_threads = scheduler->ready_threads->next;
    scheduler->thread_current = next_thread;

    return next_thread;
}

thread_t __scheduler_current()
{
    __debug
    (
        if(scheduler->thread_current == scheduler->kernel_main)
            __debug_str("Current is kernel main\n");
    );

    return scheduler->thread_current;
}

void __scheduler_user_thread_increment()
{
    __debug_str("User thread created\n");
    scheduler->user_thread_count++;
}

void __scheduler_user_thread_decrement()
{
    __debug_str("User thread exited\n");
    scheduler->user_thread_count--;
}

u64 __scheduler_user_thread_count()
{
    return scheduler->user_thread_count;
}

void __scheduler_queue_print()
{
    if(scheduler->ready_threads == 0ULL)
    {
        __print_str("Scheduler thread list empty\n");
        return;
    }

    __print_str("Scheduler thread list:\n");

    list_t *node = scheduler->ready_threads;
    do
    {
        thread_t thread = container_of(node, struct __thread_t, list_node);
        __print_mem("Thread", (u64)thread);
        node = node->next;
    }
    while(node != scheduler->ready_threads);

    __print_str("Scheduler thread list over\n");
}

void __scheduler_blocked_print()
{
    if(scheduler->sleeping_threads == 0ULL)
    {
        __print_str("Scheduler blocked thread list empty\n");
        return;
    }

    __print_str("Scheduler blocked thread list:\n");

    list_t *node = scheduler->sleeping_threads;
    do
    {
        sleeping_thread_t sleeping_thread = container_of(node, struct __sleeping_thread_t, list_node);
        __print_mem("Thread", (u64)sleeping_thread);
        __print_mem("Time delta", (u64)sleeping_thread->delta);
        node = node->next;
    }
    while(node != scheduler->sleeping_threads);

    __print_str("Scheduler blocked thread list over\n");
}

void __scheduler_timeout(thread_t thread, time_t time, sem_t semaphore)
{
    if(scheduler->sleeping_threads == 0ULL)
    {
        u64 sleeping_thread_size_in_blocks = (sizeof(struct __sleeping_thread_t) + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
        sleeping_thread_t sleeping_thread = __mem_alloc(sleeping_thread_size_in_blocks);

        sleeping_thread->thread = thread;
        sleeping_thread->semaphore = semaphore;
        sleeping_thread->delta = time;
        sleeping_thread->list_node.next = &(sleeping_thread->list_node);
        sleeping_thread->list_node.prev = &(sleeping_thread->list_node);

        scheduler->sleeping_threads = &(sleeping_thread->list_node);

        return;
    }

    list_t *node = scheduler->sleeping_threads;

    sleeping_thread_t thread_sleeping_first = container_of(node, struct __sleeping_thread_t, list_node);
    if(time < thread_sleeping_first->delta)
    {
        thread_sleeping_first->delta -= time;

        u64 sleeping_thread_size_in_blocks = (sizeof(struct __sleeping_thread_t) + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
        sleeping_thread_t new_head = __mem_alloc(sleeping_thread_size_in_blocks);

        new_head->thread = thread;
        new_head->semaphore = semaphore;
        new_head->delta = time;

        new_head->list_node.next = &(thread_sleeping_first->list_node);
        new_head->list_node.prev = thread_sleeping_first->list_node.prev;

        (thread_sleeping_first->list_node.prev)->next = &(new_head->list_node);
        thread_sleeping_first->list_node.prev = &(new_head->list_node);

        scheduler->sleeping_threads = &(new_head->list_node);

        return;
    }

    sleeping_thread_t next_thread = container_of(node, struct __sleeping_thread_t, list_node);
    time_t delta = 0ULL;

    do
    {
        delta += next_thread->delta;

        __debug_mem("Thread", (u64)thread);
        node = node->next;
        next_thread = container_of(node, struct __sleeping_thread_t, list_node);
    }
    while((node != scheduler->sleeping_threads) && (delta + next_thread->delta < time));

    u64 sleeping_thread_size_in_blocks = (sizeof(struct __sleeping_thread_t) + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
    sleeping_thread_t new_sleeping_thread = __mem_alloc(sleeping_thread_size_in_blocks);

    new_sleeping_thread->thread = thread;
    new_sleeping_thread->semaphore = semaphore;
    new_sleeping_thread->delta = time - delta;

    list_insert(node, &(new_sleeping_thread->list_node));

    if(node != scheduler->sleeping_threads)
    {
        sleeping_thread_t next = container_of(node, struct __sleeping_thread_t, list_node);
        next->delta -= new_sleeping_thread->delta;
    }
}

void __scheduler_remove_timeout(thread_t thread)
{
    if(scheduler->sleeping_threads == 0ULL)
        __panic("Thread lost 1\n");

    list_t *sleeping_threads_head = scheduler->sleeping_threads;
    sleeping_thread_t current_thread = container_of(scheduler->sleeping_threads, struct __sleeping_thread_t, list_node);

    if(sleeping_threads_head->next == sleeping_threads_head)
    {
        if(current_thread->thread != thread)
            __panic("Thread lost 2\n");

        if(__mem_free(current_thread))
            __panic("Failed to free sleeping thread struct, memory corruption\n");

        scheduler->sleeping_threads = 0ULL;
        return;
    }

    while(current_thread->thread != thread && sleeping_threads_head->next != scheduler->sleeping_threads)
    {
        sleeping_threads_head = sleeping_threads_head->next;
        current_thread = container_of(sleeping_threads_head, struct __sleeping_thread_t, list_node);
    }

    if(current_thread->thread != thread)
        __panic("Thread lost 3\n");

    if(sleeping_threads_head->next == scheduler->sleeping_threads)
    {
        sleeping_threads_head->prev->next = sleeping_threads_head->next;
        sleeping_threads_head->next->prev = sleeping_threads_head->prev;

        if(__mem_free(current_thread))
           __panic("Failed to free sleeping thread struct, memory corruption\n");

        return;
    }

    sleeping_thread_t next_thread = container_of(sleeping_threads_head->next, struct __sleeping_thread_t, list_node);
    next_thread->delta += current_thread->delta;

    sleeping_threads_head->prev->next = sleeping_threads_head->next;
    sleeping_threads_head->next->prev = sleeping_threads_head->prev;

    if(__mem_free(current_thread))
       __panic("Failed to free sleeping thread struct, memory corruption\n");

    return;
}

void __scheduler_tick()
{
    if(scheduler->sleeping_threads == 0ULL)
        return;

    sleeping_thread_t thread_sleeping_first = container_of(scheduler->sleeping_threads, struct __sleeping_thread_t, list_node);
    thread_sleeping_first->delta--;

    if(thread_sleeping_first->delta)
        return;

    while(thread_sleeping_first->delta == 0)
    {
        if(scheduler->sleeping_threads->next == scheduler->sleeping_threads)
        {
            // Remove from semaphore
            if(thread_sleeping_first->semaphore)
                __sem_remove_thread(thread_sleeping_first->semaphore, thread_sleeping_first->thread);

            thread_t thread_first = thread_sleeping_first->thread;
            __scheduler_push(thread_first);
            scheduler->sleeping_threads = 0ULL;

            if(__mem_free(thread_sleeping_first))
                __panic("Failed to free sleeping thread struct, memory corruption\n");

            return;
        }

        // Remove from semaphore
        if(thread_sleeping_first->semaphore)
            __sem_remove_thread(thread_sleeping_first->semaphore, thread_sleeping_first->thread);

        thread_t thread_first = thread_sleeping_first->thread;
        __scheduler_push(thread_first);

        scheduler->sleeping_threads->prev->next = scheduler->sleeping_threads->next;
        scheduler->sleeping_threads->next->prev = scheduler->sleeping_threads->prev;

        scheduler->sleeping_threads = scheduler->sleeping_threads->next;

        if(__mem_free(thread_sleeping_first))
            __panic("Failed to free sleeping thread struct, memory corruption\n");

        thread_sleeping_first = container_of(scheduler->sleeping_threads, struct __sleeping_thread_t, list_node);
    }

    return;
}
