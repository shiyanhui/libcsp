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

#include <stdio.h>
#include <libcsp/csp.h>

chan_declare(mm, int, mm);
chan_define(mm, int, mm);

proc void consumer(chan_t(mm) *chan, int id) {
  int num = 0;
  while (true) {
    chan_pop(chan, &num);
    printf("consumer %d received %d\n", id, num);
  }
}

proc void producer(chan_t(mm) *chan, int id, int factor) {
  int num = 0;
  while (true) {
    chan_push(chan, num * factor);
    printf("producer %d sent %d\n", id, num++ * factor);
  }
}

int main(void) {
  chan_t(mm) *chan = chan_new(mm)(3);

  async(
    producer(chan, 0, 1);
    producer(chan, 1, -1);
    consumer(chan, 0);
    consumer(chan, 1);
  );

  hangup(timer_second * 10);
  chan_destroy(chan);

  return 0;
}
