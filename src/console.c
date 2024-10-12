#include "../h/mem.h"
#include "../h/console.h"
#include "../h/list.h"
#include "../h/scheduler.h"

char *console_receive_buffer;
char *console_send_buffer;

u32 console_receive_cnt;
u32 console_send_cnt;

list_t *threads_waiting_for_input;

enum CONSOLE_ERRORS
{
    CONSOLE_RECEIVE_BUFFER_EMPTY = -1,
    CONSOLE_SEND_BUFFER_FULL = -2,
};

void __console_init()
{
    u64 console_buffer_size_in_blocks = (CONSOLE_BUFFER_SIZE + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;

    console_receive_buffer = __mem_alloc(console_buffer_size_in_blocks);
    console_send_buffer = __mem_alloc(console_buffer_size_in_blocks);

    console_receive_cnt = 0;
    console_send_cnt = 0;

    threads_waiting_for_input = 0ULL;
}

int __putc(char c)
{
    if(console_send_cnt == CONSOLE_BUFFER_SIZE)
    {
        __console_send();

        if(console_send_cnt == CONSOLE_BUFFER_SIZE)
            return CONSOLE_SEND_BUFFER_FULL;
    }

    __console_send();
    console_send_buffer[console_send_cnt] = c;
    console_send_cnt++;

    return 0;
}

void __waiting_push(thread_t thread)
{
    if(threads_waiting_for_input == 0ULL)
    {
        thread->list_node.next = &(thread->list_node);
        thread->list_node.prev = &(thread->list_node);
        threads_waiting_for_input = &(thread->list_node);

        return;
    }

    list_insert(threads_waiting_for_input, &(thread->list_node));

    return;
}

thread_t __waiting_pop()
{
    if(threads_waiting_for_input->next == threads_waiting_for_input)
    {
        thread_t next_thread = container_of(threads_waiting_for_input, struct __thread_t, list_node);
        threads_waiting_for_input = 0ULL;

        return next_thread;
    }

    thread_t next_thread = container_of(threads_waiting_for_input, struct __thread_t, list_node);

    threads_waiting_for_input->prev->next = threads_waiting_for_input->next;
    threads_waiting_for_input->next->prev = threads_waiting_for_input->prev;

    threads_waiting_for_input = threads_waiting_for_input->next;

    return next_thread;
}

int __getc()
{
    if(console_receive_cnt == 0)
    {
        thread_t thread_current = __scheduler_current();
        thread_t thread_next = __scheduler_next();

        yield(thread_current, thread_next);
        __waiting_push(thread_current);

        return CONSOLE_RECEIVE_BUFFER_EMPTY;
    }

    thread_t thread_current = __scheduler_current();
    context_t context = (context_t)(thread_current->sp);

    context->a0 = console_receive_buffer[0];
    console_receive_cnt--;

    for(u32 i = 0; i < console_receive_cnt; i++)
        console_receive_buffer[i] = console_receive_buffer[i + 1];

    return 0;
}

void __console_receive()
{
    while((*(char *)CONSOLE_STATUS & CONSOLE_STATUS_RECEIVE) && (console_receive_cnt < CONSOLE_BUFFER_SIZE))
    {
        console_receive_buffer[console_receive_cnt] = *(char *)CONSOLE_RX_DATA;
        console_receive_cnt++;
    }

    u64 unblocked_cnt = 0;
    while(threads_waiting_for_input && (unblocked_cnt < console_receive_cnt))
    {
        thread_t unblocked_thread = __waiting_pop();
        context_t context = (context_t)(unblocked_thread->sp);

        context->a0 = console_receive_buffer[unblocked_cnt];
        unblocked_cnt++;

        __scheduler_push(unblocked_thread);
    }

    console_receive_cnt -= unblocked_cnt;
    for(u64 i = 0; i < console_receive_cnt; i++)
        console_receive_buffer[i] = console_receive_buffer[unblocked_cnt + i];
}

void __console_send()
{
    u32 bytes_sent = 0;
    while((*(char *)CONSOLE_STATUS & CONSOLE_STATUS_SEND) && (bytes_sent < console_send_cnt))
    {
        *(char *)CONSOLE_TX_DATA = console_send_buffer[bytes_sent];
        bytes_sent++;
    }

    console_send_cnt -= bytes_sent;
    for(u32 i = 0; i < console_send_cnt; i++)
        console_send_buffer[i] = console_send_buffer[bytes_sent + i];
}
