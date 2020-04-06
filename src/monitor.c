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

#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include "core.h"
#include "corepool.h"
#include "netpoll.h"
#include "proc.h"
#include "rand.h"
#include "timer.h"

/* 10ms */
#define csp_monitor_max_sleep_microsecs 10000

/* Libcsp can support at most 2048 cores. */
#define csp_monitor_cores_len 2048

/* The length of csp_monitor_procs. */
#define csp_monitor_procs_len 16

/* Put the porcesses in the link list to the array and return the number of
 * processes put. */
#define csp_monitor_procs_put_list(start, max_len) ({                          \
  size_t num = 0;                                                              \
  csp_proc_t *next;                                                            \
  while ((start) != NULL && num < max_len) {                                   \
    csp_monitor_procs[num++] = (start);                                        \
    next = (start)->next;                                                      \
    (start)->next = (start)->pre = NULL;                                       \
    (start) = next;                                                            \
  }                                                                            \
  num;                                                                         \
})                                                                             \

csp_mmrbq_declare(csp_core_t *, core);

extern int csp_sched_np;
extern csp_mmrbq_t(core) *csp_sched_starving_threads, *csp_sched_starving_procs;
extern int csp_netpoll_poll(csp_proc_t **start, csp_proc_t **end);
extern int csp_timer_poll(csp_proc_t **start, csp_proc_t **end);

static csp_rand_t csp_monitor_rand;
static csp_proc_t *csp_monitor_procs[csp_monitor_procs_len];
static csp_core_t *csp_monitor_cores[csp_monitor_cores_len];

bool csp_monitor_poll(int (*poll)(csp_proc_t **, csp_proc_t **)) {
  csp_proc_t *start, *end;

  int n = poll(&start, &end);
  if (n <= 0) {
    return false;
  }

  csp_core_t *core;
  if (csp_mmrbq_try_pop(core)(csp_sched_starving_procs, &core)) {
    csp_lrunq_set(core->lrunq, n, start, end);
    csp_cond_signal(&core->pcond, csp_cond_signal_proc_avail);
    return true;
  }

  while (true) {
    size_t num = csp_monitor_procs_put_list(start, csp_monitor_procs_len);
    if (num == 0) {
      break;
    }
    int pid = csp_rand(&csp_monitor_rand) % csp_sched_np;
    while (!csp_grunq_try_pushm(csp_core_pool(pid)->grunq,
        csp_monitor_procs, num)) {
      if (csp_unlikely(++pid >= csp_sched_np)) {
        pid = 0;
      }
    }
  }

  if (csp_mmrbq_try_pop(core)(csp_sched_starving_threads, &core)) {
    csp_core_wakeup(core);
  }
  return true;
}

void *csp_monitor(void *data) {
  int64_t duration = 1, since_last_checked = 0;
  while (true) {
    if (!csp_monitor_poll(csp_netpoll_poll) &&
        !csp_monitor_poll(csp_timer_poll)) {
      since_last_checked += duration;
      usleep(duration);

      duration <<= 1;
      if (duration > csp_monitor_max_sleep_microsecs) {
        duration = csp_monitor_max_sleep_microsecs;
      }
    } else {
      duration = 1;
      /* We consider that the mointer spent `csp_monitor_max_sleep_microsecs`
       * to poll. */
      since_last_checked += csp_monitor_max_sleep_microsecs;
    }

    if (since_last_checked < csp_timer_second / 1000) {
      continue;
    }

    size_t n = csp_mmrbq_try_popm(core)(
      csp_sched_starving_procs, csp_monitor_cores, csp_monitor_cores_len
    ), len = 0;

    if (n > 0) {
      csp_timer_time_t now = csp_timer_now();
      for (size_t i = 0; i < n; i++) {
        csp_core_t *core = csp_monitor_cores[i];
        if (now - core->pcond.start > csp_timer_second) {
          /* Tell the core to go to deep sleep. */
          csp_cond_signal(&core->pcond, csp_cond_signal_deep_sleep);
        } else {
          csp_monitor_cores[len++] = core;
        }
      }
      while(!csp_mmrbq_try_pushm(core)(
        csp_sched_starving_procs, csp_monitor_cores, len
      ));
    }

    since_last_checked = 0;
  }
}

bool csp_monitor_init(void) {
  pthread_t tid;
  pthread_attr_t attr;

  if (pthread_attr_init(&attr) != 0 ||
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0 ||
    pthread_create(&tid, &attr, csp_monitor, NULL) != 0) {
    return false;
  }
  pthread_attr_destroy(&attr);

  csp_rand_init(&csp_monitor_rand);
  return true;
}
