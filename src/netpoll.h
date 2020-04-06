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

#ifndef LIBCSP_NETPOLL_H
#define LIBCSP_NETPOLL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include "proc.h"
#include "timer.h"

#define csp_netpoll_avail     csp_proc_stat_netpoll_avail
#define csp_netpoll_timeout   csp_proc_stat_netpoll_timeout

bool csp_netpoll_register(int fd);
int csp_netpoll_wait_read(int fd,  csp_timer_duration_t timeout);
int csp_netpoll_wait_write(int fd, csp_timer_duration_t timeout);
bool csp_netpoll_unregister(int fd);

#ifdef __cplusplus
}
#endif

#endif
