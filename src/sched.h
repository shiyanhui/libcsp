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

#ifndef LIBCSP_SCHED_H
#define LIBCSP_SCHED_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdatomic.h>
#include <stdbool.h>
#include "core.h"
#include "timer.h"

#define csp_sched_async(tasks)  csp_sched_run(false, tasks)
#define csp_sched_sync(tasks)   csp_sched_run(true, tasks)

#define csp_sched_run(is_sync, tasks) do {                                     \
  csp_sched_proc_anchor(is_sync);                                              \
  csp_proc_nchild_set(0);                                                      \
  { tasks; }                                                                   \
  csp_sched_yield();                                                           \
} while (0)                                                                    \

#define csp_sched_block(tasks) do {                                            \
  csp_core_t *this_core = csp_this_core;                                       \
  if (csp_core_block_prologue(this_core)) {                                    \
    { tasks; }                                                                 \
    csp_core_block_epilogue(this_core, this_core->running);                    \
  } else {                                                                     \
    { tasks; }                                                                 \
  }                                                                            \
} while (0)                                                                    \

void csp_sched_yield(void);
void csp_sched_hangup(uint64_t nanoseconds);
void csp_sched_proc_anchor(bool need_sync) __attribute__((noinline));
void csp_shced_atomic_incr(atomic_uint_fast64_t *cnt) __attribute__((noinline));

extern _Thread_local csp_core_t *csp_this_core;

#ifdef __cplusplus
}
#endif

#endif
