#include "../h/console.h"
#include "../h/kernel.h"
#include "../h/mem.h"

void __print_u64(u64 var)
{
    if(var == 0)
    {
        __putc('0');
        return;
    }

    u64 mod = 10000000000000000000ULL;

    while(var / mod == 0)
        mod /= 10;

    while(mod != 0)
    {
        __putc('0' + var / mod);
        var %= mod;
        mod /= 10;
    }
}

void __print_i32(i32 var)
{
    if(var == 0)
    {
        __putc('0');
        return;
    }

    if((u32)var & (1U << 31))
    {
        __putc('-');
        var = ~var + 1;
    }

    u32 mod = 1000000000;

    while(var / mod == 0)
        mod /= 10;

    while(mod != 0)
    {
        __putc('0' + var / mod);
        var %= mod;
        mod /= 10;
    }
}

void __print_h64(u64 var)
{
    static char hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    u64 mod = 1ULL << 60;

    while(mod != 0)
    {
        __putc(hex[var / mod]);
        var %= mod;
        mod >>= 4;
    }
}

void __print_h32(u32 var)
{
    static char hex[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    u64 mod = 1ULL << 28;

    while(mod != 0)
    {
        __putc(hex[var / mod]);
        var %= mod;
        mod >>= 4;
    }
}

void __print_str(char *str)
{
    while(*str)
        __putc(*(str++));
}

void __print_mem(char *msg, u64 mem)
{
    __print_str(msg);
    __print_str(": ");
    __print_u64(mem);
    __print_str(" : ");
    __print_h64(mem);
    __putc('\n');
}

void __stop()
{
    __console_send();

    ASM("li t0, 0x5555");
    ASM("li t1, 0x100000");
    ASM("sw t0, (t1)");
}

void __panic(char *msg)
{
    __console_send();
    __mem_print();

    __print_str("KERNEL PANIC!\n");
    __print_str(msg);

    __stop();
}
