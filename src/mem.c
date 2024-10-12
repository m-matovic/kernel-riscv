#include "../h/mem.h"
#include "../h/kernel.h"

i32 *mem_index;
u64 n_mem_blocks;
u64 mem_index_len_in_blocks;

enum FREE_ERRORS
{
    FREE_OUT_OF_HEAP = -1,
    FREE_NOT_BLOCK_ALLIGNED = -2,
    FREE_NOT_ALLOCED = -3,
    FREE_NOT_START_OF_ALLOC = -4,
};

void __mem_init()
{
    n_mem_blocks = (HEAP_END_ADDR - HEAP_START_ADDR) / MEM_BLOCK_SIZE;
    mem_index_len_in_blocks = (sizeof(u32) * n_mem_blocks + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;

    mem_index = (i32 *)HEAP_START_ADDR;

    for(u64 i = 0; i < n_mem_blocks; i++)
        mem_index[i] = 0;

    mem_index[0] = mem_index_len_in_blocks;
    mem_index[mem_index_len_in_blocks - 1] = mem_index_len_in_blocks;
    mem_index[mem_index_len_in_blocks] = -(n_mem_blocks - mem_index_len_in_blocks);
    mem_index[n_mem_blocks - 1] = -(n_mem_blocks - mem_index_len_in_blocks);
}

void __mem_print()
{
    for(u64 i = 0; i < n_mem_blocks; i++)
    {
        if(mem_index[i] != 0)
        {
            __print_i32(mem_index[i]);
            __print_str(" ");
        }
    }

    __print_str("\n");
}

void *__mem_alloc(size_t nblocks)
{
    u64 i = 0;

    while(i < n_mem_blocks)
    {
        if(mem_index[i] > 0)
            i += mem_index[i];

        if(mem_index[i] < 0)
        {
            i32 free_blocks_count = -mem_index[i];

            if(free_blocks_count > nblocks)
            {
                mem_index[i + free_blocks_count - 1] = -(free_blocks_count - nblocks);
                mem_index[i + nblocks] = -(free_blocks_count - nblocks);
                mem_index[i + nblocks - 1] = nblocks;
                mem_index[i] = nblocks;

                //Cast avoids warning about losing const
                return (void *)(HEAP_START_ADDR + i * MEM_BLOCK_SIZE);
            }

            if(free_blocks_count == nblocks)
            {
                mem_index[i] = nblocks;
                mem_index[i + nblocks - 1] = nblocks;

                //Cast avoids warning about losing const
                return (void *)(HEAP_START_ADDR + i * MEM_BLOCK_SIZE);
            }

            i += free_blocks_count;
        }

        if(mem_index[i] == 0)
            __panic("Memory index corrupted!\n");
    }

    return 0;
}

int __mem_free(void *ptr)
{
    u64 abs_ptr = (u64)ptr;
    u64 mem_index_entry = (abs_ptr - (u64)HEAP_START_ADDR) / MEM_BLOCK_SIZE;

    if(abs_ptr < (u64)HEAP_START_ADDR || abs_ptr >= (u64)HEAP_END_ADDR)
        return FREE_OUT_OF_HEAP;

    if((abs_ptr - (u64)HEAP_START_ADDR) % MEM_BLOCK_SIZE)
        return FREE_NOT_BLOCK_ALLIGNED;

    if(mem_index[mem_index_entry] <= 0)
        return FREE_NOT_ALLOCED;

    if(mem_index[mem_index_entry] != mem_index[mem_index_entry + mem_index[mem_index_entry] - 1])
        return FREE_NOT_START_OF_ALLOC;

    i32 size = mem_index[mem_index_entry];
    u8 prev_free = mem_index_entry > 0 ? (mem_index[mem_index_entry - 1] < 0) : 0;
    u8 next_free = mem_index_entry + size < n_mem_blocks ? (mem_index[mem_index_entry + size] < 0) : 0;

    if(!prev_free && !next_free)
    {
        mem_index[mem_index_entry] = -size;
        mem_index[mem_index_entry + size - 1] = -size;
    }

    if(prev_free)
    {
        u64 prev_index = mem_index_entry + mem_index[mem_index_entry - 1];
        i32 joined_size = size - mem_index[mem_index_entry - 1];

        mem_index[mem_index_entry] = 0;
        mem_index[mem_index_entry - 1] = 0;
        mem_index[prev_index] = -joined_size;
        mem_index[prev_index + joined_size - 1] = -joined_size;

        mem_index_entry = prev_index;
        size = joined_size;
    }

    if(next_free)
    {
        i32 joined_size = size - mem_index[mem_index_entry + size];

        mem_index[mem_index_entry + size - 1] = 0;
        mem_index[mem_index_entry + size] = 0;
        mem_index[mem_index_entry] = -joined_size;
        mem_index[mem_index_entry + joined_size - 1] = -joined_size;
    }

    return 0;
}
