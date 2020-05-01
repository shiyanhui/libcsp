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

#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <libcsp/csp.h>

chan_declare(mm, int, int);
chan_define(mm, int, int);

chan_declare(mm, char, char);
chan_define(mm, char, char);

proc void choose(chan_t(int) *chn1, chan_t(char) *chn2) {
  srand(time(NULL));

  /* We can make the simulation of `select` in golang. The only drawback of this
   * simulation is the priority. We always check the channels by the order of
   * which we write in the source code. But it's enough in most cases. */
  while (true) {
    bool ok = false;

    int num;
    if (csp_chan_try_pop(chn1, &num)) {
      printf("chn1 received number %d\n", num);
      ok = true;
    }

    if (csp_chan_try_push(chn2, rand() & 0x7f)) {
      ok = true;
    }

    /* If all channels failed, it's up to you to give up the control of the CPU
     * or break the loop or do some other things. Actually it's the `default`
     * case of the `select`. */
    if (!ok) {
      yield();
    }
  }
}

proc void send_int(chan_t(int) *chn) {
  int num = 0;
  while (true) {
    csp_chan_push(chn, num++);
  }
}

proc void receive_char(chan_t(char) *chn) {
  char chr;
  while (true) {
    csp_chan_pop(chn, &chr);
    printf("chn2 received char %c\n", chr);
  }
}

int main(void) {
  chan_t(int) *chn1 = chan_new(int)(3);
  chan_t(char) *chn2 = chan_new(char)(3);

  sync(choose(chn1, chn2); send_int(chn1); receive_char(chn2));

  chan_destroy(chn1);
  chan_destroy(chn2);
  return 0;
}
