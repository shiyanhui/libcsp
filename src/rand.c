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

#include <stdlib.h>
#include <time.h>
#include "rand.h"

#define csp_rand_rol64(x, k)  (((x) << (k)) | ((x) >> (64 - (k))))

void csp_rand_init(csp_rand_t *r) {
  srand(time(NULL));
  for (int i = 0; i < sizeof(r->state) / sizeof(uint64_t); i++) {
    r->state[i] = rand();
  }
  csp_mutex_init(&r->mutex);
}

uint64_t csp_rand(csp_rand_t *r) {
  uint64_t *s = (r)->state,
    t = s[1] << 17,
    ret = csp_rand_rol64(s[1] * 5, 7) * 9;

  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];

  s[2] ^= t;
  s[3] = csp_rand_rol64(s[3], 45);
  return ret;
}
