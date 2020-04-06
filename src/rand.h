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

#ifndef LIBCSP_RAND_H
#define LIBCSP_RAND_H

#include <stdint.h>
#include "mutex.h"

/*
 * `csp_rand_t` implements the `xoshiro256**` algorithm.
 * See https://en.wikipedia.org/wiki/Xorshift#xoshiro256**.
 */
typedef struct {
  uint64_t state[4];
  csp_mutex_t mutex;
} csp_rand_t;

/* `csp_rand_init` initializes the random number generator. It is NOT
 * thread-safe. */
void csp_rand_init(csp_rand_t *r);

/* `csp_rand` gets the next random number. It is NOT thread-safe. */
uint64_t csp_rand(csp_rand_t *r);

#endif
