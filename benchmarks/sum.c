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

#define N 10

proc void sum(int64_t low, int64_t high, int64_t *result) {
  if (low == high) {
    *result = low;
    return;
  }
  int64_t mid = low + ((high - low) >> 1), left, right;
  sync(sum(low, mid, &left); sum(mid + 1, high, &right));
  *result = left + right;
}

int main(void) {
  int64_t result = 0;
  timer_time_t start, end;

  start = timer_now();
  for (int i = 0; i < N; i++) {
    sum(0, 10000000, &result);
  }
  end = timer_now();

  printf("The result is %ld, ran %d rounds, %lf seconds per round.\n",
    result, N, (double)(end - start) / timer_second / N
  );
  return 0;
}
