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
#include <fcntl.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <unistd.h>
#include "core.h"
#include "netpoll.h"
#include "proc.h"
#include "runq.h"
#include "timer.h"

#define csp_netpoll_waiter_proc_get(w)    atomic_load(&(w)->proc)
#define csp_netpoll_waiter_proc_set(w, p) atomic_store(&(w)->proc, (p))

#define csp_netpoll_wait(fd, timeout, evt) ({                                  \
  csp_core_t *this_core = csp_this_core;                                       \
                                                                               \
  csp_proc_t *running = this_core->running;                                    \
  csp_proc_stat_set(running, csp_proc_stat_netpoll_waiting);                   \
                                                                               \
  /* Set waiter to tell netpoll we are waiting the evt. */                     \
  csp_netpoll_waiter_t *waiter = &csp_netpoll.waiters[fd];                     \
  csp_netpoll_waiter_proc_set(waiter, running);                                \
  waiter->waiting_evt = (evt);                                                 \
                                                                               \
  if ((timeout) > 0) {                                                         \
    csp_timer_t timer = csp_timer_after(                                       \
      (timeout), csp_netpoll_on_timeout((fd), running)                         \
    );                                                                         \
    csp_soft_mbarr();                                                          \
    waiter->timer = &timer;                                                    \
  } else {                                                                     \
    /* Tell the netpoll there is no timer. */                                  \
    waiter->timer = NULL;                                                      \
  }                                                                            \
                                                                               \
  /* Set this_core->running to NULL to prevent this process being scheduled    \
   * twice. */                                                                 \
  this_core->running = NULL;                                                   \
  csp_core_yield(running, &this_core->anchor);                                 \
                                                                               \
  csp_netpoll_waiter_proc_set(waiter, NULL);                                   \
  csp_proc_stat_get(running);                                                  \
})                                                                             \

extern _Thread_local csp_core_t *csp_this_core;
extern void csp_core_proc_exit_and_run(csp_proc_t *to_run);
extern void csp_core_yield(csp_proc_t *proc, void *anchor);

typedef struct {
  /* Whether is fd is registered to the netpoll. */
  bool registered;

  /* The event(EPOLLIN or EPOLLOUT) we are waiting. */
  int waiting_evt;

  /* The process waiting for the event. `NULL` means there is no waiting
   * process. */
  csp_proc_t *proc;

  /* The timer if timeout is set. */
  csp_timer_t *timer;
} csp_netpoll_waiter_t;

struct {
  int epfd, waiters_cap;
  csp_netpoll_waiter_t *waiters;
  struct epoll_event evts[128];
} csp_netpoll;

bool csp_netpoll_init(void) {
  struct rlimit r;
  if (getrlimit(RLIMIT_NOFILE, &r) == -1 || r.rlim_max <= 0) {
    return false;
  }

  csp_netpoll.waiters_cap = r.rlim_max;
  csp_netpoll.waiters = (csp_netpoll_waiter_t *)calloc(
     csp_netpoll.waiters_cap, sizeof(csp_netpoll_waiter_t)
  );
  if (csp_netpoll.waiters == NULL) {
    return false;
  }

  csp_netpoll.epfd = epoll_create1(0);
  return csp_netpoll.epfd != -1;
}

bool csp_netpoll_register(int fd) {
  /* Set the socket to be non-blocking. */
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags != -1) {
    flags = fcntl(fd, F_SETFL, flags|O_NONBLOCK);
  }
  if (flags == -1) {
    return false;
  }

  struct epoll_event evt = {
    .events = EPOLLET|EPOLLIN|EPOLLOUT,
    .data = {.fd = fd}
  };
  if (epoll_ctl(csp_netpoll.epfd, EPOLL_CTL_ADD, fd, &evt) == -1) {
    return false;
  }

  csp_netpoll.waiters[fd].registered = true;
  return true;
}

/* The timeout handler. */
csp_proc static void csp_netpoll_on_timeout(int fd, csp_proc_t *proc) {
  uint64_t stat = csp_proc_stat_netpoll_waiting;
  if (csp_proc_stat_cas(proc, stat, csp_proc_stat_netpoll_timeout)) {
    csp_core_proc_exit_and_run(proc);
  }
}

int csp_netpoll_wait_read(int fd, csp_timer_duration_t timeout) {
  return csp_netpoll_wait(fd, timeout, EPOLLIN);
}

int csp_netpoll_wait_write(int fd, csp_timer_duration_t timeout) {
  return csp_netpoll_wait(fd, timeout, EPOLLOUT);
}

int csp_netpoll_poll(csp_proc_t **start, csp_proc_t **end) {
  int n = epoll_wait(csp_netpoll.epfd, csp_netpoll.evts,
    sizeof(csp_netpoll.evts)/sizeof(struct epoll_event), 0
  );
  if (n <= 0) {
    return 0;
  }

  int len = 0;
  csp_proc_t *head = NULL, *tail = NULL;

  for (int i = 0; i < n; i++) {
    csp_netpoll_waiter_t *waiter = &csp_netpoll.waiters[
      csp_netpoll.evts[i].data.fd
    ];

    csp_proc_t *proc = csp_netpoll_waiter_proc_get(waiter);
    if (proc == NULL) {
      continue;
    }

    uint32_t mask = 0;
    if (csp_netpoll.evts[i].events & EPOLLIN) {
      mask |= EPOLLIN;
    }
    if (csp_netpoll.evts[i].events & EPOLLOUT) {
      mask |= EPOLLOUT;
    }
    /* Regard EPOLLERR and EPOLLHUP as success. The caller should handle the
     * error in the following read or write. */
    if (csp_netpoll.evts[i].events & (EPOLLERR|EPOLLHUP)) {
      mask |= EPOLLIN|EPOLLOUT;
    }

    uint64_t stat = csp_proc_stat_netpoll_waiting;
    if ((mask & waiter->waiting_evt) &&
        csp_proc_stat_cas(proc, stat, csp_proc_stat_netpoll_avail)) {
      if (waiter->timer != NULL) {
        csp_timer_cancel(*waiter->timer);
      }

      if (tail != NULL) {
        tail->next = proc;
        proc->pre = tail;
        tail = proc;
      } else {
        head = tail = proc;
      }
      len++;
    }
  }

  if (len > 0) {
    *start = head;
    *end = tail;
  }

  return len;
}

bool csp_netpoll_unregister(int fd) {
  if (epoll_ctl(csp_netpoll.epfd, EPOLL_CTL_DEL, fd, NULL) == -1) {
    return false;
  }
  csp_netpoll.waiters[fd].registered = false;
  return true;
}

void csp_netpoll_destroy() {
  for (int i = 0; i < csp_netpoll.waiters_cap; i++) {
    if (csp_netpoll.waiters[i].registered) {
      csp_netpoll_unregister(i);
    }
  }
  free(csp_netpoll.waiters);
  close(csp_netpoll.epfd);
}
