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

#ifndef LIBCSP_CPOOL_H
#define LIBCSP_CPOOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include "core.h"
#include "mutex.h"

#define csp_core_pool(i) (csp_core_pools.pools[i])

typedef struct {
  size_t cap, top;
  csp_core_t **cores;
  csp_lrunq_t *lrunq;
  csp_grunq_t *grunq;
  csp_mutex_t mutex;
} csp_core_pool_t;

struct { size_t len; csp_core_pool_t **pools; } csp_core_pools;

#ifdef __cplusplus
}
#endif

#endif
