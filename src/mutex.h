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

#ifndef LIBCSP_MUTEX_H
#define LIBCSP_MUTEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdatomic.h>

#define csp_mutex_t                atomic_flag
#define csp_mutex_try_lock(mutex)  (!atomic_flag_test_and_set(mutex))
#define csp_mutex_lock(mutex)      while (!csp_mutex_try_lock(mutex)) {}
#define csp_mutex_unlock(mutex)    atomic_flag_clear(mutex)
#define csp_mutex_init(mutex)                                                  \
  do { *(mutex) = (atomic_flag)ATOMIC_FLAG_INIT; } while (0)                   \

#ifdef __cplusplus
}
#endif

#endif
