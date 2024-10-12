#ifndef MEM_HEADER
#define MEM_HEADER

#include "../lib/hw.h"
#include "../h/kernel.h"

void __mem_init();
void *__mem_alloc(size_t nblocks);
int __mem_free(void *ptr);
void __mem_print();

#endif //MEM_HEADER
