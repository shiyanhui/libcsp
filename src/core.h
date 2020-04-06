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

#ifndef LIBCSP_CORE_H
#define LIBCSP_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include "cond.h"
#include "proc.h"
#include "runq.h"

#define csp_core_state_set(c, s)    atomic_store(&(c)->state, (s))
#define csp_core_state_get(c)       atomic_load(&(c)->state)
#define csp_core_state_cas(c, o, n)                                            \
  atomic_compare_exchange_weak(&(c)->state, &(o), n)                           \

#define csp_core_wakeup(core) do {                                             \
  pthread_mutex_lock(&(core)->mutex);                                          \
  pthread_cond_signal(&(core)->cond);                                          \
  pthread_mutex_unlock(&(core)->mutex);                                        \
} while (0)                                                                    \

typedef enum {
  csp_core_state_inited,
  csp_core_state_running,
} csp_core_state_t;

typedef struct {
  /*
   * `anchor` is used to save the the thread context in `csp_core_run`. So when
   * a process finishes or yields, we can switch to this context and find the
   * next process to run.
   *
   * NOTE: This is should be the first field of `csp_core_t` cause we used this
   * `csp_core_run`.
   */
  struct { int64_t rbp, rsp, rip, rbx; } anchor;

  /* Current process running on the core. */
  csp_proc_t *running;

  /* Id of the thread the core runs on. */
  pthread_t tid;

  /* The id of cpu processor with which the core binds. */
  size_t pid;

  /* State of the core. */
  _Atomic csp_core_state_t state;

  /* The local runq used by cores running on the same processor. */
  csp_lrunq_t *lrunq;

  /* The global runq used by cores running on the same processor. */
  csp_grunq_t *grunq;

  /* `mutex` and `cond` are used to do synchronization operations. */
  pthread_mutex_t mutex;
  pthread_cond_t cond;

  /* porc-level conditional variable. */
  csp_cond_t pcond;
} csp_core_t;

bool csp_core_block_prologue(csp_core_t *core);
void csp_core_block_epilogue(csp_core_t *core, csp_proc_t *proc)
__attribute__((naked));

#ifdef __cplusplus
}
#endif

#endif
