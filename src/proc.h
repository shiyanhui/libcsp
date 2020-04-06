/*
 * Copyright (c) 2020, Yanhui Shi <lime.syh at gmail dot com>
 * All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef LIBCSP_PROC_H
#define LIBCSP_PROC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef csp_enable_valgrind
#include <valgrind/valgrind.h>
#endif

#define csp_proc                        __attribute__((used,noinline))
#define csp_proc_nchild_get(p)          atomic_load(&(p)->nchild)
#define csp_proc_nchild_incr(p)         atomic_fetch_add(&(p)->nchild, 1)
#define csp_proc_nchild_decr(p)         atomic_fetch_sub(&(p)->nchild, 1)
#define csp_proc_timer_token_get(p)     atomic_load(&((p)->timer.token))
#define csp_proc_timer_token_set(p, v)  atomic_store(&((p)->timer.token), (v))
#define csp_proc_timer_token_cas(p, old, new)                                  \
  atomic_compare_exchange_weak(&((p)->timer.token), &old, new)

#define csp_proc_stat_none              0
#define csp_proc_stat_netpoll_waiting   1
#define csp_proc_stat_netpoll_avail     2
#define csp_proc_stat_netpoll_timeout   3
#define csp_proc_stat_get(proc)         atomic_load(&(proc)->stat)
#define csp_proc_stat_set(proc, val)    atomic_store(&(proc)->stat, val)
#define csp_proc_stat_cas(proc, oval, nval)                                    \
  atomic_compare_exchange_weak(&(proc)->stat, &(oval), nval)

#define csp_proc_save(reg)                                                     \
  "stmxcsr   0x18(%"reg")\n"                                                   \
  "fstcw     0x1c(%"reg")\n"                                                   \
  "mov %rsp, 0x20(%"reg")\n"                                                   \
  "mov %rbp, 0x28(%"reg")\n"                                                   \
  "mov %rbx, 0x30(%"reg")\n"                                                   \
  "mov %r12, 0x38(%"reg")\n"                                                   \
  "mov %r13, 0x40(%"reg")\n"                                                   \
  "mov %r14, 0x48(%"reg")\n"                                                   \
  "mov %r15, 0x50(%"reg")\n"                                                   \

/*
 * Memory layout of the process is:
 *
 *  ← Low Address                                         High Address →
 *  +------------------------------------------------------------------+
 *  | Stack | Return Address | Memory Arguments | Padding | csp_proc_t |
 *  +------------------------------------------------------------------+
 *          |←        stack_usage_t.proc_reserved        →|
 *
 *  - csp_proc_t: The context of the process.
 *
 *  - Padding: According to System V x86_64 ABI the 3.2.2 section:
 *
 *      > The end of the input argument area shall be aligned on a 16 (32 or 64,
 *      > if __m256 or __m512 is passed on stack) byte boundary. In other words,
 *      > the value (%rsp + 8) is always a multiple of 16 (32 or 64) when control
 *      > is transferred to the function entry point.
 *
 *      See `https://github.com/hjl-tools/x86-psABI/wiki/x86-64-psABI-1.0.pdf`.
 *
 *    We may make some padding to ensure that after copying the argument passed
 *    by stack the `%rsp` meets `%rsp mod 16 = 0`.
 *
 *  - Memory Arguments: The arguments passed by stack. The order is the same
 *    that defined in System V x86_64 ABI, i.e. right-to-left.
 *
 *  - Return Address: The return address in the stack frame.
 *
 *  - Stack: The stack of the process.
 *
 *  NOTE:
 *
 *  - Actually `Return Address` and `Memory Arguments` is a part of the stack
 *    frame.
 *  - The size of `csp_proc_t` is used in `stack_analyzer_t.analyze()` of
 *    `plugin/sa.hpp`, so if we add fields to or remove fields from the struct
 *    `csp_proc_t`, DO NOT forget to modify the variable `csp_proc_t_size`
 *    in `stack_analyzer_t.analyze()` manually.
 */
typedef struct csp_proc_t {
  /* The address malloced from operating system. */
  uint64_t base;

  /* The id of CPU processor on which this process is created. */
  uint64_t borned_pid;

  /* Whether the process is a new proces.
   *
   * When it is 1, it means it have not ran, and the scheduler should restore
   * the caller-saved registers.
   *
   * When it is 0, it means the process has ran at least once, and the scheduler
   * should restore the callee-saved registers. */
  uint64_t is_new;

  /* The MXCSR register. */
  uint32_t mxcsr;

  /* The x87 FPU Control Word. */
  uint32_t x87cw;

  /* The rsp register. */
  uint64_t rsp;

  /* The rbp register. */
  uint64_t rbp;

  /* Callee-saved or caller-saved registers depending on `is_new`. */
  union {
    struct { uint64_t rbx, r12, r13, r14, r15; } callee_saved;
    /* There is no need to save other caller-saved registers(e.g. rax, r10 and
     * r11 etc.). */
    struct { uint64_t rdi, rsi, rdx, rcx, r8, r9; } caller_saved;
  } registers;

  /* The Timer information. */
  struct { int64_t when, idx; atomic_int_fast64_t token; } timer;

  /* The waiting parent process. */
  struct csp_proc_t *parent;

  /* Used in lrunq. */
  struct csp_proc_t *pre, *next;

  /* Number of children the porcess is waiting. */
  atomic_uint_fast64_t nchild;

  /* The state of process. */
  atomic_uint_fast64_t stat;

#ifdef csp_enable_valgrind
  /* The id returned by VALGRIND_STACK_REGISTER. */
  uint64_t valgrind_stack;
#endif
} csp_proc_t;

void csp_proc_nchild_set(size_t nchild);

#ifdef __cplusplus
}
#endif

#endif
