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

#include <assert.h>
#include <string.h>
#include "../src/rbq.h"

#define CAP_EXP     3
#define CAP         (1 << CAP_EXP)

csp_ssrbq_declare(int, ss);
csp_ssrbq_define(int, ss);

csp_smrbq_declare(int, sm);
csp_smrbq_define(int, sm);

csp_msrbq_declare(int, ms);
csp_msrbq_define(int, ms);

csp_mmrbq_declare(int, mm);
csp_mmrbq_define(int, mm);

csp_rrbq_declare(int, r);
csp_rrbq_define(int, r);

void csp_sched_yield(void) {}

int array[] = {8, 7, 6, 5, 4, 3, 2, 1};
int array_len = sizeof(array) / sizeof(int);
int array_cpy[sizeof(array) / sizeof(int)];

void test_ssrbq(void) {
  csp_ssrbq_t(ss) *rbq = csp_ssrbq_new(ss)(CAP_EXP);
  for (int i = 0; i < CAP; i++) {
    assert(csp_ssrbq_try_push(ss)(rbq, i));
  }
  assert(!csp_ssrbq_try_push(ss)(rbq, -1));
  assert(!csp_ssrbq_try_pushm(ss)(rbq, array, array_len));

  int val;
  for (int i = 0; i < CAP; i++) {
    assert(csp_ssrbq_try_pop(ss)(rbq, &val));
    assert(val == i);
  }
  assert(!csp_ssrbq_try_pop(ss)(rbq, &val));
  assert(csp_ssrbq_try_popm(ss)(rbq, array_cpy, array_len) == 0);

  for (int i = 0; i < CAP; i++) {
    csp_ssrbq_push(ss)(rbq, i);
  }
  assert(!csp_ssrbq_try_push(ss)(rbq, -1));
  assert(!csp_ssrbq_try_pushm(ss)(rbq, array, array_len));

  for (int i = 0; i < CAP; i++) {
    csp_ssrbq_pop(ss)(rbq, &val);
    assert(val == i);
  }
  assert(!csp_ssrbq_try_pop(ss)(rbq, &val));
  assert(csp_ssrbq_try_popm(ss)(rbq, array_cpy, array_len) == 0);

  assert(csp_ssrbq_try_pushm(ss)(rbq, array, array_len));
  assert(!csp_ssrbq_try_push(ss)(rbq, -1));
  assert(!csp_ssrbq_try_pushm(ss)(rbq, array, array_len));

  assert(csp_ssrbq_try_popm(ss)(rbq, array_cpy, array_len) == array_len);
  assert(memcmp(array, array_cpy, sizeof(array)) == 0);
  assert(!csp_ssrbq_try_pop(ss)(rbq, &val));
  assert(csp_ssrbq_try_popm(ss)(rbq, array_cpy, array_len) == 0);

  memset(array_cpy, 0, sizeof(array));

  csp_ssrbq_pushm(ss)(rbq, array, array_len);
  assert(!csp_ssrbq_try_push(ss)(rbq, -1));
  assert(!csp_ssrbq_try_pushm(ss)(rbq, array, array_len));

  csp_ssrbq_popm(ss)(rbq, array_cpy, array_len);
  assert(memcmp(array, array_cpy, sizeof(array)) == 0);
  assert(!csp_ssrbq_try_pop(ss)(rbq, &val));
  assert(csp_ssrbq_try_popm(ss)(rbq, array_cpy, array_len) == 0);

  csp_ssrbq_destroy(ss)(rbq);
}

void test_smrbq(void) {
  csp_smrbq_t(sm) *rbq = csp_smrbq_new(sm)(CAP_EXP);
  for (int i = 0; i < CAP; i++) {
    assert(csp_smrbq_try_push(sm)(rbq, i));
  }
  assert(!csp_smrbq_try_push(sm)(rbq, -1));
  assert(!csp_smrbq_try_pushm(sm)(rbq, array, array_len));

  int val;
  for (int i = 0; i < CAP; i++) {
    assert(csp_smrbq_try_pop(sm)(rbq, &val));
    assert(val == i);
  }
  assert(!csp_smrbq_try_pop(sm)(rbq, &val));
  assert(csp_smrbq_try_popm(sm)(rbq, array_cpy, array_len) == 0);

  for (int i = 0; i < CAP; i++) {
    csp_smrbq_push(sm)(rbq, i);
  }
  assert(!csp_smrbq_try_push(sm)(rbq, -1));
  assert(!csp_smrbq_try_pushm(sm)(rbq, array, array_len));

  for (int i = 0; i < CAP; i++) {
    csp_smrbq_pop(sm)(rbq, &val);
    assert(val == i);
  }
  assert(!csp_smrbq_try_pop(sm)(rbq, &val));
  assert(csp_smrbq_try_popm(sm)(rbq, array_cpy, array_len) == 0);

  assert(csp_smrbq_try_pushm(sm)(rbq, array, array_len));
  assert(!csp_smrbq_try_push(sm)(rbq, -1));
  assert(!csp_smrbq_try_pushm(sm)(rbq, array, array_len));

  assert(csp_smrbq_try_popm(sm)(rbq, array_cpy, array_len) == array_len);
  assert(memcmp(array, array_cpy, sizeof(array)) == 0);
  assert(!csp_smrbq_try_pop(sm)(rbq, &val));
  assert(csp_smrbq_try_popm(sm)(rbq, array_cpy, array_len) == 0);

  memset(array_cpy, 0, sizeof(array));

  csp_smrbq_pushm(sm)(rbq, array, array_len);
  assert(!csp_smrbq_try_push(sm)(rbq, -1));
  assert(!csp_smrbq_try_pushm(sm)(rbq, array, array_len));

  csp_smrbq_popm(sm)(rbq, array_cpy, array_len);
  assert(memcmp(array, array_cpy, sizeof(array)) == 0);
  assert(!csp_smrbq_try_pop(sm)(rbq, &val));
  assert(csp_smrbq_try_popm(sm)(rbq, array_cpy, array_len) == 0);

  csp_smrbq_destroy(sm)(rbq);
}

void test_msrbq(void) {
  csp_msrbq_t(ms) *rbq = csp_msrbq_new(ms)(CAP_EXP);
  for (int i = 0; i < CAP; i++) {
    assert(csp_msrbq_try_push(ms)(rbq, i));
  }
  assert(!csp_msrbq_try_push(ms)(rbq, -1));
  assert(!csp_msrbq_try_pushm(ms)(rbq, array, array_len));

  int val;
  for (int i = 0; i < CAP; i++) {
    assert(csp_msrbq_try_pop(ms)(rbq, &val));
    assert(val == i);
  }
  assert(!csp_msrbq_try_pop(ms)(rbq, &val));
  assert(csp_msrbq_try_popm(ms)(rbq, array_cpy, array_len) == 0);

  for (int i = 0; i < CAP; i++) {
    csp_msrbq_push(ms)(rbq, i);
  }
  assert(!csp_msrbq_try_push(ms)(rbq, -1));
  assert(!csp_msrbq_try_pushm(ms)(rbq, array, array_len));

  for (int i = 0; i < CAP; i++) {
    csp_msrbq_pop(ms)(rbq, &val);
    assert(val == i);
  }
  assert(!csp_msrbq_try_pop(ms)(rbq, &val));
  assert(csp_msrbq_try_popm(ms)(rbq, array_cpy, array_len) == 0);

  assert(csp_msrbq_try_pushm(ms)(rbq, array, array_len));
  assert(!csp_msrbq_try_push(ms)(rbq, -1));
  assert(!csp_msrbq_try_pushm(ms)(rbq, array, array_len));

  assert(csp_msrbq_try_popm(ms)(rbq, array_cpy, array_len) == array_len);
  assert(memcmp(array, array_cpy, sizeof(array)) == 0);
  assert(!csp_msrbq_try_pop(ms)(rbq, &val));
  assert(csp_msrbq_try_popm(ms)(rbq, array_cpy, array_len) == 0);

  memset(array_cpy, 0, sizeof(array));

  csp_msrbq_pushm(ms)(rbq, array, array_len);
  assert(!csp_msrbq_try_push(ms)(rbq, -1));
  assert(!csp_msrbq_try_pushm(ms)(rbq, array, array_len));

  csp_msrbq_popm(ms)(rbq, array_cpy, array_len);
  assert(memcmp(array, array_cpy, sizeof(array)) == 0);
  assert(!csp_msrbq_try_pop(ms)(rbq, &val));
  assert(csp_msrbq_try_popm(ms)(rbq, array_cpy, array_len) == 0);

  csp_msrbq_destroy(ms)(rbq);
}

void test_mmrbq(void) {
  csp_mmrbq_t(mm) *rbq = csp_mmrbq_new(mm)(CAP_EXP);
  for (int i = 0; i < CAP; i++) {
    assert(csp_mmrbq_try_push(mm)(rbq, i));
  }
  assert(!csp_mmrbq_try_push(mm)(rbq, -1));
  assert(!csp_mmrbq_try_pushm(mm)(rbq, array, array_len));

  int val;
  for (int i = 0; i < CAP; i++) {
    assert(csp_mmrbq_try_pop(mm)(rbq, &val));
    assert(val == i);
  }
  assert(!csp_mmrbq_try_pop(mm)(rbq, &val));
  assert(csp_mmrbq_try_popm(mm)(rbq, array_cpy, array_len) == 0);

  for (int i = 0; i < CAP; i++) {
    csp_mmrbq_push(mm)(rbq, i);
  }
  assert(!csp_mmrbq_try_push(mm)(rbq, -1));
  assert(!csp_mmrbq_try_pushm(mm)(rbq, array, array_len));

  for (int i = 0; i < CAP; i++) {
    csp_mmrbq_pop(mm)(rbq, &val);
    assert(val == i);
  }
  assert(!csp_mmrbq_try_pop(mm)(rbq, &val));
  assert(csp_mmrbq_try_popm(mm)(rbq, array_cpy, array_len) == 0);

  assert(csp_mmrbq_try_pushm(mm)(rbq, array, array_len));
  assert(!csp_mmrbq_try_push(mm)(rbq, -1));
  assert(!csp_mmrbq_try_pushm(mm)(rbq, array, array_len));

  assert(csp_mmrbq_try_popm(mm)(rbq, array_cpy, array_len) == array_len);
  assert(memcmp(array, array_cpy, sizeof(array)) == 0);
  assert(!csp_mmrbq_try_pop(mm)(rbq, &val));
  assert(csp_mmrbq_try_popm(mm)(rbq, array_cpy, array_len) == 0);

  memset(array_cpy, 0, sizeof(array));

  csp_mmrbq_pushm(mm)(rbq, array, array_len);
  assert(!csp_mmrbq_try_push(mm)(rbq, -1));
  assert(!csp_mmrbq_try_pushm(mm)(rbq, array, array_len));

  csp_mmrbq_popm(mm)(rbq, array_cpy, array_len);
  assert(memcmp(array, array_cpy, sizeof(array)) == 0);
  assert(!csp_mmrbq_try_pop(mm)(rbq, &val));
  assert(csp_mmrbq_try_popm(mm)(rbq, array_cpy, array_len) == 0);

  csp_mmrbq_destroy(mm)(rbq);
}

void test_rrbq(void) {
  csp_rrbq_t(r) *rbq = csp_rrbq_new(r)(CAP_EXP);
  for (int i = 0; i < CAP; i++) {
    assert(csp_rrbq_try_push(r)(rbq, i));
  }
  assert(!csp_rrbq_try_push(r)(rbq, -1));

  int val;
  for (int i = 0; i < CAP; i++) {
    assert(csp_rrbq_try_pop(r)(rbq, &val));
    assert(val == i);
  }
  assert(!csp_rrbq_try_pop(r)(rbq, &val));

  csp_rrbq_destroy(r)(rbq);
}

int main(void) {
  test_ssrbq();
  test_smrbq();
  test_msrbq();
  test_mmrbq();
  test_rrbq();
}
