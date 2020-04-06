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

#ifndef LIBCSP_COND_H
#define LIBCSP_COND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdatomic.h>
#include "timer.h"

#define csp_cond_signal_none       0
#define csp_cond_signal_proc_avail 1
#define csp_cond_signal_deep_sleep 2

typedef struct {
  atomic_int stat;
  atomic_bool waiting;
  csp_timer_time_t start;
} csp_cond_t;

#define csp_cond_init(cond) do {                                               \
  atomic_store(&(cond)->stat, csp_cond_signal_none);                           \
  atomic_store(&(cond)->waiting, false);                                       \
  (cond)->start = 0;                                                           \
} while (0)                                                                    \

#define csp_cond_before_wait(cond) do {                                        \
  (cond)->start = csp_timer_now();                                             \
} while (0)                                                                    \

#define csp_cond_wait(cond) ({                                                 \
  int signal;                                                                  \
  while ((signal = atomic_load(&(cond)->stat)) == csp_cond_signal_none) {      \
    atomic_store(&(cond)->waiting, true);                                      \
  }                                                                            \
  csp_cond_init(cond);                                                         \
  signal;                                                                      \
})                                                                             \

#define csp_cond_signal(cond, signal) do {                                     \
  while (!atomic_load(&(cond)->waiting));                                      \
  atomic_store(&(cond)->stat, (signal));                                       \
} while(0)                                                                     \

#ifdef __cplusplus
}
#endif

#endif
