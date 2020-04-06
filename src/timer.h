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

#ifndef LIBCSP_TIMER_H
#define LIBCSP_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "common.h"
#include "proc.h"

#define csp_timer_nanosecond    ((csp_timer_duration_t)1)
#define csp_timer_microsecond   (csp_timer_nanosecond * 1000)
#define csp_timer_millisecond   (csp_timer_microsecond * 1000)
#define csp_timer_second        (csp_timer_millisecond * 1000)
#define csp_timer_minute        (csp_timer_second * 60)
#define csp_timer_hour          (csp_timer_minute * 60)

/* `csp_timer_now` gets current time. */
#define csp_timer_now() ({                                                     \
  struct timespec ts;                                                          \
  clock_gettime(CLOCK_REALTIME, &ts);                                          \
  (csp_timer_time_t)(ts.tv_sec * csp_timer_second + ts.tv_nsec);               \
})                                                                             \

/* `csp_timer_at` sets a timer triggered at `when` in nanoseconds. */
#define csp_timer_at(when, task) ({                                            \
  csp_soft_mbarr();                                                            \
  csp_timer_t timer;                                                           \
  csp_timer_anchor(when);                                                      \
  task;                                                                        \
  __asm__ __volatile__("mov %%rax, %0\n" :"=r"(timer.ctx) :: "rax", "memory"); \
  timer.token = csp_proc_timer_token_get(timer.ctx);                           \
  csp_soft_mbarr();                                                            \
  timer;                                                                       \
})                                                                             \

/* `csp_timer_after` sets a timer triggered after `duration` nanoseconds. */
#define csp_timer_after(duration, task)                                        \
  csp_timer_at(csp_timer_now() + (duration), (task))                           \

/* `csp_timer_time_t` is the timestamp in nanoseconds. */
typedef int64_t csp_timer_time_t;

/* `csp_timer_duration_t` is the time duration in nanoseconds. */
typedef int64_t csp_timer_duration_t;

/* `csp_timer_t` implements the timer. */
typedef struct { csp_proc_t *ctx; int64_t token; } csp_timer_t;

/* `csp_timer_cancel` cancels the timer. Return true if success. */
bool csp_timer_cancel(csp_timer_t timer);

void csp_timer_anchor(csp_timer_time_t when);

#ifdef __cplusplus
}
#endif

#endif
