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

proc void say(const char *content) {
  printf("%s\n", content);
}

int main(void) {
  timer_after(timer_second, say("Hello world!"));

  timer_at(
    timer_now() + timer_second * 2,
    say("Hello world again!")
  );

  timer_t timer = timer_after(
    timer_second * 2,
    say("This will not be printed.")
  );

  /* Cancel the timer after one second, so it will not be triggered. */
  hangup(timer_second);
  assert(timer_cancel(timer));

  /* Wait one more second to make sure we have enough time to observe it. */
  hangup(timer_second * 2);

  return 0;
}
