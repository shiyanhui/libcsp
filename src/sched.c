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

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include "common.h"
#include "core.h"
#include "corepool.h"
#include "rbq.h"
#include "runq.h"
#include "sched.h"
#include "timer.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

extern size_t csp_cpu_cores;
extern _Thread_local csp_core_t *csp_this_core;

extern void csp_core_init_main(csp_core_t *core);
extern bool csp_core_start(csp_core_t *core);
extern bool csp_core_pools_init(void);
extern bool csp_core_pools_get(size_t pid, csp_core_t **core);
extern void csp_core_pools_destroy(void);
extern void csp_core_yield(csp_proc_t *proc, void *anchor);
extern bool csp_monitor_init(void);
extern bool csp_netpoll_init(void);
extern bool csp_timer_heaps_init(void);
extern void csp_timer_heaps_destroy(void);
extern void csp_timer_put(size_t pid, csp_proc_t *proc);

#ifndef csp_with_sysmalloc
extern bool csp_mem_init(void);
extern void csp_mem_destroy(void);
#endif

csp_mmrbq_declare(csp_core_t *, core);
csp_mmrbq_define(csp_core_t *, core);

int csp_sched_np;
csp_mmrbq_t(core) *csp_sched_starving_threads, *csp_sched_starving_procs;

__attribute__((constructor)) static void csp_sched_start() {
  /* Get the number of processores. */
  csp_sched_np = sysconf(_SC_NPROCESSORS_ONLN);
  if (csp_sched_np  <= 0) {
    csp_sched_np = 1;
  }

  if (csp_cpu_cores > 0 && csp_cpu_cores < csp_sched_np) {
    csp_sched_np = csp_cpu_cores;
  }

  csp_sched_starving_threads = csp_mmrbq_new(core)(csp_exp(csp_sched_np));
  if (csp_sched_starving_threads == NULL) {
    errno = ENOMEM;
    perror("Failed to initialize starving queue.");
    exit(EXIT_FAILURE);
  }

  csp_sched_starving_procs = csp_mmrbq_new(core)(csp_exp(csp_sched_np));
  if (csp_sched_starving_procs == NULL) {
    errno = ENOMEM;
    perror("Failed to initialize starving queue.");
    exit(EXIT_FAILURE);
  }

  if (!csp_core_pools_init()) {
    errno = ENOMEM;
    perror("Failed to initialize core pools.");
    exit(EXIT_FAILURE);
  }

#ifndef csp_with_sysmalloc
  if (!csp_mem_init()) {
    errno = ENOMEM;
    perror("Failed to initialize mm.");
    exit(EXIT_FAILURE);
  }
#endif

  if (!csp_netpoll_init()) {
    perror("Failed to initialize netpoll.");
    exit(EXIT_FAILURE);
  }

  if (!csp_timer_heaps_init()) {
    errno = ENOMEM;
    perror("Failed to initialize timer heaps.");
    exit(EXIT_FAILURE);
  }

  if (!csp_monitor_init()) {
    perror("Failed to initialize monitor.");
    exit(EXIT_FAILURE);
  }

  csp_core_t *core;
  for (size_t i = 0; i < csp_sched_np; i++) {
    csp_core_pools_get(i, &core);
    if (i == 0) {
      csp_core_init_main(core);
    } else if (!csp_core_start(core)) {
      perror("Failed to start thread.");
      exit(EXIT_FAILURE);
    }
  }
}

void csp_sched_put_proc(csp_proc_t *proc) {
  csp_lrunq_push_front(csp_this_core->lrunq, proc);
}

/* We must return the proc cause we may use it in `csp_timer_cancel`. */
csp_proc_t *csp_sched_put_timer(csp_proc_t *proc) {
  csp_timer_put(csp_this_core->pid, proc);
  return proc;
}

csp_proc_t *csp_sched_get(csp_core_t *this_core) {
  int pid, code;
  csp_proc_t *running = this_core->running, *proc;

  while (true) {
    code = csp_lrunq_try_pop_front(this_core->lrunq, &proc);
    if (code == csp_lrunq_ok || (code == csp_lrunq_missed && (
        csp_grunq_try_pop(csp_core_pool(this_core->pid)->grunq, &proc) ||
        csp_lrunq_try_pop_front(this_core->lrunq, &proc) == csp_lrunq_ok))) {
      goto found;
    }

    pid = this_core->pid;
    for (int i = 0; i < csp_sched_np; i++) {
      if (csp_grunq_try_pop(csp_core_pool(pid)->grunq, &proc)) {
        goto found;
      }
      if (++pid == csp_sched_np) {
        pid = 0;
      }
    }

    /* If stealing failed, we continue to run current proc if it's valid. */
    if (running != NULL && csp_proc_nchild_get(running) == 0) {
      return running;
    }

    /* We must call this before push it to the starving queue, otherwise there
     * will be data race with monitor. */
    csp_cond_before_wait(&this_core->pcond);

    while(!csp_mmrbq_try_push(core)(csp_sched_starving_procs, this_core));
    if (csp_cond_wait(&this_core->pcond) == csp_cond_signal_deep_sleep) {
      pthread_mutex_lock(&this_core->mutex);
      while(!csp_mmrbq_try_push(core)(csp_sched_starving_threads, this_core));
      pthread_cond_wait(&this_core->cond, &this_core->mutex);
      pthread_mutex_unlock(&this_core->mutex);
    }
  }

found:
  if (running != NULL && csp_proc_nchild_get(running) == 0) {
    csp_lrunq_push(this_core->lrunq, running);
  }

  size_t half = (csp_lrunq_len(this_core->lrunq) + 1) >> 1;
  if (half == 0) {
    return proc;
  }

  csp_core_t *starving_core;
  if (csp_mmrbq_try_pop(core)(csp_sched_starving_procs, &starving_core)) {
    csp_proc_t *start = NULL, *end = NULL;
    csp_lrunq_popm_front(this_core->lrunq, half, &start, &end);
    csp_lrunq_set(starving_core->lrunq, half, start, end);
    csp_cond_signal(&starving_core->pcond, csp_cond_signal_proc_avail);
  }
  return proc;
}

void csp_sched_yield(void) {
  csp_core_t *this_core = csp_this_core;
  csp_core_yield(this_core->running, &this_core->anchor);
}

void csp_sched_hangup(uint64_t nanoseconds) {
  if (csp_unlikely(nanoseconds == 0)) {
    return;
  }

  csp_core_t *this_core = csp_this_core;
  csp_proc_t *running = this_core->running;
  running->timer.when = csp_timer_now() + nanoseconds;
  csp_timer_put(this_core->pid, running);

  // Set `this_core->running` to zero to prevent it be scheduled.
  this_core->running = NULL;
  csp_core_yield(running, &this_core->anchor);
}

__attribute__((noinline)) void csp_sched_proc_anchor(bool need_sync) {};

__attribute__((noinline))
void csp_shced_atomic_incr(atomic_uint_fast64_t *cnt) {};

/* TODO: Directly release the resource without overhead may panic, so currently
 * we only rely on the OS to take back the resource. */
__attribute__((destructor)) static void csp_sched_stop(void) {
  /* csp_core_pools_destroy(); */
  /* csp_timer_events_pool_destroy(); */
/* #ifndef csp_with_sysmalloc */
  /* csp_mem_destroy(); */
/* #endif */
}
