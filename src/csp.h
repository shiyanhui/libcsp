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

#ifndef LIBCSP_H
#define LIBCSP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "chan.h"
#include "mutex.h"
#include "netpoll.h"
#include "sched.h"
#include "timer.h"

#define csp_async   csp_sched_async
#define csp_sync    csp_sched_sync
#define csp_block   csp_sched_block
#define csp_yield   csp_sched_yield
#define csp_hangup  csp_sched_hangup

/* All */
#ifdef csp_without_prefix
#ifndef csp_chan_without_prefix
#define csp_chan_without_prefix
#endif

#ifndef csp_mutex_without_prefix
#define csp_mutex_without_prefix
#endif

#ifndef csp_netpoll_without_prefix
#define csp_netpoll_without_prefix
#endif

#ifndef csp_sched_without_prefix
#define csp_sched_without_prefix
#endif

#ifndef csp_timer_without_prefix
#define csp_timer_without_prefix
#endif
#endif

/* Channel */
#ifdef csp_chan_without_prefix
#define chan_t              csp_chan_t
#define chan_new            csp_chan_new
#define chan_try_push       csp_chan_try_push
#define chan_push           csp_chan_push
#define chan_try_pop        csp_chan_try_pop
#define chan_pop            csp_chan_pop
#define chan_try_pushm      csp_chan_try_pushm
#define chan_pushm          csp_chan_pushm
#define chan_try_popm       csp_chan_try_popm
#define chan_popm           csp_chan_popm
#define chan_destroy        csp_chan_destroy
#define chan_declare        csp_chan_declare
#define chan_define         csp_chan_define
#endif

/* Mutex */
#ifdef csp_mutex_without_prefix
#define mutex_t             csp_mutex_t
#define mutex_try_lock      csp_mutex_try_lock
#define mutex_lock          csp_mutex_lock
#define mutex_unlock        csp_mutex_unlock
#define mutex_init          csp_mutex_init
#endif

/* Netpoll */
#ifdef csp_netpoll_without_prefix
#define netpoll_avail       csp_proc_stat_netpoll_avail
#define netpoll_timeout     csp_proc_stat_netpoll_timeout
#define netpoll_register    csp_netpoll_register
#define netpoll_wait_read   csp_netpoll_wait_read
#define netpoll_wait_write  csp_netpoll_wait_write
#define netpoll_unregister  csp_netpoll_unregister
#endif

/* Schedule */
#ifdef csp_sched_without_prefix
#define proc                csp_proc
#define async               csp_async
#define sync                csp_sync
#define block               csp_block
#define yield               csp_yield
#define hangup              csp_hangup
#endif

/* Timer */
#ifdef csp_timer_without_prefix
#define timer_nanosecond    csp_timer_nanosecond
#define timer_microsecond   csp_timer_microsecond
#define timer_millisecond   csp_timer_millisecond
#define timer_second        csp_timer_second
#define timer_minute        csp_timer_minute
#define timer_hour          csp_timer_hour
#define timer_time_t        csp_timer_time_t
#define timer_duration_t    csp_timer_duration_t
#define timer_t             csp_timer_t
#define timer_now           csp_timer_now
#define timer_at            csp_timer_at
#define timer_after         csp_timer_after
#define timer_cancel        csp_timer_cancel
#endif

#ifdef __cplusplus
}
#endif

#endif
