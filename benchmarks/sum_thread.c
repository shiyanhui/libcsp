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

#include <libcsp/timer.h>
#include <pthread.h>
#include <stdio.h>

#define N 10

// We set MAX to 10^4 since if we set it to 10^7 the program will panic caused
// by exceeding the linux max thread limit(I increased it manually, but it still
// fails.) in my machine.
//
// It's extremely slow though. The result is something like,
//
//   > The result is 50005000, ran 10 rounds, 3.200744 seconds per round.
//
#define MAX 10000

typedef struct { int64_t low, high, result; } args_t;

void *sum(void *args) {
  args_t *data = (args_t *)args;
  if (data->low == data->high) {
    data->result = data->low;
    return NULL;
  }

  int64_t mid = data->low + ((data->high - data->low) >> 1);
  args_t
    args1 = {.low = data->low, .high = mid, .result = -1},
    args2 = {.low = mid + 1, .high = data->high, .result = -1};

  pthread_t tid1, tid2;
  pthread_create(&tid1, NULL, sum, &args1);
  pthread_create(&tid2, NULL, sum, &args2);
  pthread_join(tid1, NULL);
  pthread_join(tid2, NULL);

  data->result = args1.result + args2.result;

  return NULL;
}

int main(void) {
  args_t result = {.low = 0, .high = MAX, .result = -1};
  csp_timer_time_t start, end;

  start = csp_timer_now();
  for (int i = 0; i < N; i++) {
    sum(&result);
  }
  end = csp_timer_now();

  printf("The result is %ld, ran %d rounds, %lf seconds per round.\n",
    result.result, N, (double)(end - start) / csp_timer_second / N
  );
  return 0;
}
