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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <pthread.h>
#include "../src/chan.h"

#define CAP_EXP     3
#define CAP         (1 << CAP_EXP)

csp_chan_declare(ss, int, ss);
csp_chan_define(ss, int, ss);

csp_chan_declare(sm, int, sm);
csp_chan_define(sm, int, sm);

csp_chan_declare(ms, int, ms);
csp_chan_define(ms, int, ms);

csp_chan_declare(mm, int, mm);
csp_chan_define(mm, int, mm);

void csp_sched_yield(void) {}

int array[] = {8, 7, 6, 5, 4, 3, 2, 1};
int array_len = sizeof(array) / sizeof(int);
int array_cpy[sizeof(array) / sizeof(int)];

void test_chan_ss(void) {
  csp_chan_t(ss) *chan = csp_chan_new(ss)(CAP_EXP);
  for (int i = 0; i < CAP; i++) {
    assert(csp_chan_try_push(chan, i));
  }
  assert(!csp_chan_try_push(chan, -1));
  assert(!csp_chan_try_pushm(chan, array, array_len));

  int val;
  for (int i = 0; i < CAP; i++) {
    assert(csp_chan_try_pop(chan, &val));
    assert(val == i);
  }
  assert(!csp_chan_try_pop(chan, &val));
  assert(csp_chan_try_popm(chan, array_cpy, array_len) == 0);

  for (int i = 0; i < CAP; i++) {
    csp_chan_push(chan, i);
  }
  assert(!csp_chan_try_push(chan, -1));
  assert(!csp_chan_try_pushm(chan, array, array_len));

  for (int i = 0; i < CAP; i++) {
    csp_chan_pop(chan, &val);
    assert(val == i);
  }
  assert(!csp_chan_try_pop(chan, &val));
  assert(csp_chan_try_popm(chan, array_cpy, array_len) == 0);

  assert(csp_chan_try_pushm(chan, array, array_len));
  assert(!csp_chan_try_push(chan, -1));
  assert(!csp_chan_try_pushm(chan, array, array_len));

  assert(csp_chan_try_popm(chan, array_cpy, array_len) == array_len);
  assert(memcmp(array, array_cpy, sizeof(array)) == 0);
  assert(!csp_chan_try_pop(chan, &val));
  assert(csp_chan_try_popm(chan, array_cpy, array_len) == 0);

  memset(array_cpy, 0, sizeof(array));

  csp_chan_pushm(chan, array, array_len);
  assert(!csp_chan_try_push(chan, -1));
  assert(!csp_chan_try_pushm(chan, array, array_len));

  csp_chan_popm(chan, array_cpy, array_len);
  assert(memcmp(array, array_cpy, sizeof(array)) == 0);
  assert(!csp_chan_try_pop(chan, &val));
  assert(csp_chan_try_popm(chan, array_cpy, array_len) == 0);

  csp_chan_destroy(chan);
}

void test_chan_sm(void) {
  csp_chan_t(sm) *chan = csp_chan_new(sm)(CAP_EXP);
  for (int i = 0; i < CAP; i++) {
    assert(csp_chan_try_push(chan, i));
  }
  assert(!csp_chan_try_push(chan, -1));
  assert(!csp_chan_try_pushm(chan, array, array_len));

  int val;
  for (int i = 0; i < CAP; i++) {
    assert(csp_chan_try_pop(chan, &val));
    assert(val == i);
  }
  assert(!csp_chan_try_pop(chan, &val));
  assert(csp_chan_try_popm(chan, array_cpy, array_len) == 0);

  for (int i = 0; i < CAP; i++) {
    csp_chan_push(chan, i);
  }
  assert(!csp_chan_try_push(chan, -1));
  assert(!csp_chan_try_pushm(chan, array, array_len));

  for (int i = 0; i < CAP; i++) {
    csp_chan_pop(chan, &val);
    assert(val == i);
  }
  assert(!csp_chan_try_pop(chan, &val));
  assert(csp_chan_try_popm(chan, array_cpy, array_len) == 0);

  assert(csp_chan_try_pushm(chan, array, array_len));
  assert(!csp_chan_try_push(chan, -1));
  assert(!csp_chan_try_pushm(chan, array, array_len));

  assert(csp_chan_try_popm(chan, array_cpy, array_len) == array_len);
  assert(memcmp(array, array_cpy, sizeof(array)) == 0);
  assert(!csp_chan_try_pop(chan, &val));
  assert(csp_chan_try_popm(chan, array_cpy, array_len) == 0);

  memset(array_cpy, 0, sizeof(array));

  csp_chan_pushm(chan, array, array_len);
  assert(!csp_chan_try_push(chan, -1));
  assert(!csp_chan_try_pushm(chan, array, array_len));

  csp_chan_popm(chan, array_cpy, array_len);
  assert(memcmp(array, array_cpy, sizeof(array)) == 0);
  assert(!csp_chan_try_pop(chan, &val));
  assert(csp_chan_try_popm(chan, array_cpy, array_len) == 0);

  csp_chan_destroy(chan);
}

void test_chan_ms(void) {
  csp_chan_t(ms) *chan = csp_chan_new(ms)(CAP_EXP);
  for (int i = 0; i < CAP; i++) {
    assert(csp_chan_try_push(chan, i));
  }
  assert(!csp_chan_try_push(chan, -1));
  assert(!csp_chan_try_pushm(chan, array, array_len));

  int val;
  for (int i = 0; i < CAP; i++) {
    assert(csp_chan_try_pop(chan, &val));
    assert(val == i);
  }
  assert(!csp_chan_try_pop(chan, &val));
  assert(csp_chan_try_popm(chan, array_cpy, array_len) == 0);

  for (int i = 0; i < CAP; i++) {
    csp_chan_push(chan, i);
  }
  assert(!csp_chan_try_push(chan, -1));
  assert(!csp_chan_try_pushm(chan, array, array_len));

  for (int i = 0; i < CAP; i++) {
    csp_chan_pop(chan, &val);
    assert(val == i);
  }
  assert(!csp_chan_try_pop(chan, &val));
  assert(csp_chan_try_popm(chan, array_cpy, array_len) == 0);

  assert(csp_chan_try_pushm(chan, array, array_len));
  assert(!csp_chan_try_push(chan, -1));
  assert(!csp_chan_try_pushm(chan, array, array_len));

  assert(csp_chan_try_popm(chan, array_cpy, array_len) == array_len);
  assert(memcmp(array, array_cpy, sizeof(array)) == 0);
  assert(!csp_chan_try_pop(chan, &val));
  assert(csp_chan_try_popm(chan, array_cpy, array_len) == 0);

  memset(array_cpy, 0, sizeof(array));

  csp_chan_pushm(chan, array, array_len);
  assert(!csp_chan_try_push(chan, -1));
  assert(!csp_chan_try_pushm(chan, array, array_len));

  csp_chan_popm(chan, array_cpy, array_len);
  assert(memcmp(array, array_cpy, sizeof(array)) == 0);
  assert(!csp_chan_try_pop(chan, &val));
  assert(csp_chan_try_popm(chan, array_cpy, array_len) == 0);

  csp_chan_destroy(chan);
}

void test_chan_mm(void) {
  csp_chan_t(mm) *chan = csp_chan_new(mm)(CAP_EXP);
  for (int i = 0; i < CAP; i++) {
    assert(csp_chan_try_push(chan, i));
  }
  assert(!csp_chan_try_push(chan, -1));
  assert(!csp_chan_try_pushm(chan, array, array_len));

  int val;
  for (int i = 0; i < CAP; i++) {
    assert(csp_chan_try_pop(chan, &val));
    assert(val == i);
  }
  assert(!csp_chan_try_pop(chan, &val));
  assert(csp_chan_try_popm(chan, array_cpy, array_len) == 0);

  for (int i = 0; i < CAP; i++) {
    csp_chan_push(chan, i);
  }
  assert(!csp_chan_try_push(chan, -1));
  assert(!csp_chan_try_pushm(chan, array, array_len));

  for (int i = 0; i < CAP; i++) {
    csp_chan_pop(chan, &val);
    assert(val == i);
  }
  assert(!csp_chan_try_pop(chan, &val));
  assert(csp_chan_try_popm(chan, array_cpy, array_len) == 0);

  assert(csp_chan_try_pushm(chan, array, array_len));
  assert(!csp_chan_try_push(chan, -1));
  assert(!csp_chan_try_pushm(chan, array, array_len));

  assert(csp_chan_try_popm(chan, array_cpy, array_len) == array_len);
  assert(memcmp(array, array_cpy, sizeof(array)) == 0);
  assert(!csp_chan_try_pop(chan, &val));
  assert(csp_chan_try_popm(chan, array_cpy, array_len) == 0);

  memset(array_cpy, 0, sizeof(array));

  csp_chan_pushm(chan, array, array_len);
  assert(!csp_chan_try_push(chan, -1));
  assert(!csp_chan_try_pushm(chan, array, array_len));

  csp_chan_popm(chan, array_cpy, array_len);
  assert(memcmp(array, array_cpy, sizeof(array)) == 0);
  assert(!csp_chan_try_pop(chan, &val));
  assert(csp_chan_try_popm(chan, array_cpy, array_len) == 0);

  csp_chan_destroy(chan);
}

void *producer(void *data) {
  csp_chan_t(mm) *chan = (csp_chan_t(mm) *)(data);
  for (int i = 0; i < (1 << 25); i++) {
    while(!csp_chan_try_push(chan, i)) {}
  }
  return NULL;
}

void *consumer(void *data) {
  int val = -1;
  csp_chan_t(mm) *chan = (csp_chan_t(mm) *)(data);
  for (int i = 0; i < (1 << 25); i++) {
    while(!csp_chan_try_pop(chan, &val)) {}
    assert(val == i);
  }
  return NULL;
}

void start_thread(pthread_t *tid, int pid, void *(*fn)(void *), void *data) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(pid, &cpuset);

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);
  pthread_create(tid, &attr, fn, data);
  pthread_attr_destroy(&attr);
}

void test_chan_ss_thread(void) {
  csp_chan_t(mm) *chan = csp_chan_new(mm)(10);

  pthread_t t1, t2;
  start_thread(&t1, 0, producer, chan);
  start_thread(&t2, 1, consumer, chan);

  pthread_join(t1, NULL);
  pthread_join(t2, NULL);

  csp_chan_destroy(chan);
}

int main(void) {
  test_chan_ss_thread();
  test_chan_ss();
  test_chan_sm();
  test_chan_ms();
  test_chan_mm();
}
