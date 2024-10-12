#include "../h/semaphore.h"
#include "../h/scheduler.h"

enum SEM_CREATE_ERRORS
{
    SEM_CREATE_NO_MEMORY = -1,
};

enum SEM_WAIT_ERRORS
{
    SEM_CLOSED_EXTERNALLY = -1,
    SEM_WAIT_TIMEOUT = -2,
};

enum SEM_TRY_WAIT_ERRORS
{
    SEM_TRY_WAIT_FAILED = 1,
};

enum SEM_SIGNAL_ERRORS
{
    SEM_SIGNAL_NO_THREADS = -1,
};

sem_t first_semaphore = 0ULL;
sem_t semaphores[10];
u64 sem_cnt = 0;

void __sem_all_print()
{
    __print_mem("Semaphore count", sem_cnt);
    for(u64 i = 0; i < sem_cnt; i++)
    {
        sem_t semaphore = semaphores[i];

        __print_str("Semaphore ");
        __print_u64(i);
        __print_str(" ");
        __print_i32(semaphore->val);
        __print_str("\n");


        if(semaphore->waiting_threads == 0ULL)
        {
            __print_str("Semaphore waiting thread list empty\n");
            continue;
        }

        __print_str("Semaphore waiting thread list:\n");

        list_t *node = semaphore->waiting_threads;
        do
        {
            thread_t thread = container_of(node, struct __thread_t, list_node);
            __print_mem("Thread", (u64)thread);
            node = node->next;
        }
        while(node != semaphore->waiting_threads);

        __print_str("Semaphore waiting thread list over\n");
    }
}

void __sem_first_print()
{
    if(first_semaphore == 0ULL)
    {
        __print_str("No semaphores yet\n");
        return;
    }

    if(first_semaphore->waiting_threads == 0ULL)
    {
        __print_str("Semaphore waiting thread list empty\n");
        return;
    }

    __print_str("Semaphore waiting thread list:\n");

    list_t *node = first_semaphore->waiting_threads;
    do
    {
        thread_t thread = container_of(node, struct __thread_t, list_node);
        __print_mem("Thread", (u64)thread);
        node = node->next;
    }
    while(node != first_semaphore->waiting_threads);

    __print_str("Semaphore waiting thread list over\n");
}

int __sem_open(sem_t *handle, u32 init)
{
    u64 semaphore_size_in_blocks = (sizeof(struct __sem_t) + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
    sem_t new_sem = (sem_t)__mem_alloc(semaphore_size_in_blocks);

    __debug_mem("New semaphore", (u64)new_sem);

    if(new_sem == 0ULL)
        return SEM_CREATE_NO_MEMORY;

    new_sem->waiting_threads = 0ULL;
    new_sem->init = init;
    new_sem->val = init;
    *handle = new_sem;

    if(first_semaphore == 0ULL)
        first_semaphore = new_sem;

    semaphores[sem_cnt] = new_sem;
    sem_cnt++;

    return 0;
}

int __sem_close(sem_t handle)
{
    if(handle->waiting_threads != 0ULL)
    {
        list_t *first_thread = handle->waiting_threads;
        list_t *current_thread = first_thread;
        list_t *next_thread = first_thread;

        do
        {
            next_thread = current_thread->next;
            thread_t thread = container_of(current_thread, struct __thread_t, list_node);
            context_t context = (context_t)(thread->sp);

            __scheduler_push(thread);
            context->a0 = SEM_CLOSED_EXTERNALLY;

            current_thread = next_thread;
        }
        while(next_thread != first_thread);
    }

    if(__mem_free(handle))
        __panic("Failed to free semaphore, memory corruption\n");

    return 0;
}

void __sem_push(sem_t handle, thread_t thread)
{
    __debug_mem("Waiting thread", (u64)thread);

    if(handle->waiting_threads == 0ULL)
    {
        thread->list_node.next = &(thread->list_node);
        thread->list_node.prev = &(thread->list_node);
        handle->waiting_threads = &(thread->list_node);

        return;
    }

    list_insert(handle->waiting_threads, &(thread->list_node));

    return;
}

int __sem_wait(sem_t handle)
{
    handle->val--;

    if(handle->val < 0)
    {
        thread_t thread_current = __scheduler_current();
        thread_t thread_next = __scheduler_next();

        yield(thread_current, thread_next);
        __sem_push(handle, thread_current);

        return 1;
    }

    return 0;
}

thread_t __sem_next(sem_t handle)
{
    if(handle->waiting_threads->next == handle->waiting_threads)
    {
        thread_t next_thread = container_of(handle->waiting_threads, struct __thread_t, list_node);
        handle->waiting_threads = 0ULL;

        return next_thread;
    }

    thread_t next_thread = container_of(handle->waiting_threads, struct __thread_t, list_node);

    handle->waiting_threads->prev->next = handle->waiting_threads->next;
    handle->waiting_threads->next->prev = handle->waiting_threads->prev;

    handle->waiting_threads = handle->waiting_threads->next;

    return next_thread;
}

int __sem_unblock_thread(sem_t handle)
{
    thread_t unblocked_thread = __sem_next(handle);

    if(unblocked_thread->time_left == 0)
        __scheduler_remove_timeout(unblocked_thread);

    context_t context = (context_t)(unblocked_thread->sp);
    context->a0 = 0;

    __scheduler_push(unblocked_thread);

    return 0;
}

int __sem_signal(sem_t handle)
{
    handle->val++;

    if(handle->val <= 0)
    {
        if(handle->waiting_threads == 0ULL)
            return SEM_SIGNAL_NO_THREADS;

        __sem_unblock_thread(handle);
    }

    return 0;
}

int __sem_timed_wait(sem_t handle, time_t time)
{
    thread_t thread_current = __scheduler_current();
    thread_current->time_left = 0ULL;

    __scheduler_timeout(thread_current, time, handle);

    return __sem_wait(handle);
}

void __sem_remove_thread(sem_t handle, thread_t thread)
{
    list_t *head = handle->waiting_threads;

    if(head == 0ULL)
        __panic("Thread lost");

    handle->val++;

    if(head == head->next)
    {
        handle->waiting_threads = 0ULL;
        return;
    }

    thread_t current_thread = container_of(head, struct __thread_t, list_node);
    while(head != &current_thread->list_node)
        head = head->next;

    head->prev->next = head->next;
    head->next->prev = head->prev;

    context_t context = (context_t)(thread->sp);
    context->a0 = SEM_WAIT_TIMEOUT;

    return;
}
