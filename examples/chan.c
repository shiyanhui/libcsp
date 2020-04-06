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

#define csp_without_prefix

#include <assert.h>
#include <stdio.h>
#include <libcsp/csp.h>

chan_declare(ss, int, ss);
chan_define(ss, int, ss);

int main(void) {
  chan_t(ss) *chan = chan_new(ss)(6);

  for (int i = 0; i < 10; i++) {
    chan_push(chan, i);
    printf("%d pushed\n", i);
  }

  int val = -1, i = 0;
  while (chan_try_pop(chan, &val)) {
    assert(val == i++);
    printf("%d poped\n", val);
  }

  chan_destroy(chan);
  return 0;
}
