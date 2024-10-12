#ifndef KERNEL_HEADER
#define KERNEL_HEADER

#include "../lib/hw.h"

typedef char i8;
typedef short i16;
typedef int  i32;
typedef long i64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int  u32;
typedef unsigned long u64;

#define ASM(__inline_asm) __asm__ __volatile__ (__inline_asm)

// #define DEBUG

void __print_u64(u64);
void __print_i32(i32);
void __print_h64(u64);
void __print_h32(u32);
void __print_str(char *str);
void __print_mem(char *msg, u64 mem);

#ifdef DEBUG
#define __debug_u64(u64) __print_u64(u64)
#define __debug_i32(i32) __print_i32(i32)
#define __debug_h64(u64) __print_h64(u64)
#define __debug_h32(u32) __print_h32(u32)
#define __debug_str(str) __print_str(str)
#define __debug_mem(msg, mem) __print_mem(msg, mem)
#else
#define __debug_u64(u64) do {} while(0)
#define __debug_i32(i32) do {} while(0)
#define __debug_h64(u64) do {} while(0)
#define __debug_h32(u32) do {} while(0)
#define __debug_str(str) do {} while(0)
#define __debug_mem(msg, mem) do {} while(0)
#endif

#ifdef DEBUG
#define __debug(X) X
#else
#define __debug(X) do {} while(0)
#endif

struct __context_t
{
    u64 zero;
    u64 ra;
    u64 sp;
    u64 gp;
    u64 tp;
    u64 t0;
    u64 t1;
    u64 t2;
    u64 s0;
    u64 s1;
    u64 a0;
    u64 a1;
    u64 a2;
    u64 a3;
    u64 a4;
    u64 a5;
    u64 a6;
    u64 a7;
    u64 s2;
    u64 s3;
    u64 s4;
    u64 s5;
    u64 s6;
    u64 s7;
    u64 s8;
    u64 s9;
    u64 s10;
    u64 s11;
    u64 t3;
    u64 t4;
    u64 t5;
    u64 t6;
};

typedef struct __context_t * context_t;

void __stop();
void __panic(char *msg);

#endif //KERNEL_HEADER
