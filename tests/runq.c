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

#define csp_with_sysmalloc

#include <assert.h>
#include "../src/proc.c"
#include "../src/runq.c"

_Thread_local csp_core_t *csp_this_core = &(csp_core_t){.pid = 0};
size_t csp_procs_num = 1;
size_t csp_procs_size[] = {4096};

void csp_sched_yield(void) {}

void test_lrunq(void) {
  size_t cap_exp = 3, cap = 1 << cap_exp;
  csp_proc_t *proc = NULL;

  csp_lrunq_t *runq = csp_lrunq_new();
  assert(csp_lrunq_len(runq) == 0);
  assert(!csp_lrunq_try_pop_front(runq, &proc));

  csp_proc_t *proc1 = csp_proc_new(0, false);
  csp_proc_t *proc2 = csp_proc_new(0, false);
  csp_proc_t *proc3 = csp_proc_new(0, false);

  csp_lrunq_push(runq, proc1);
  csp_lrunq_push(runq, proc2);
  csp_lrunq_push(runq, proc3);

  /* Test the order of poped processes. */
  assert(csp_lrunq_try_pop_front(runq, &proc) && proc == proc1);
  assert(csp_lrunq_try_pop_front(runq, &proc) && proc == proc2);
  assert(csp_lrunq_try_pop_front(runq, &proc) && proc == proc3);
  assert(!csp_lrunq_try_pop_front(runq, &proc));

  csp_proc_destroy(proc1);
  csp_proc_destroy(proc2);
  csp_proc_destroy(proc3);

  csp_lrunq_destroy(runq);
}

void test_grunq(void) {
  size_t cap_exp = 3, cap = 1 << cap_exp;
  csp_proc_t *proc = (csp_proc_t *)-1;

  csp_grunq_t *grunq = csp_grunq_new(cap_exp);

  assert(!csp_grunq_try_pop(grunq, &proc));
  for (int64_t i = 0; i < cap; i++) {
    assert(csp_grunq_try_push(grunq, (csp_proc_t *)i));
  }
  assert(!csp_grunq_try_push(grunq, 0));

  for (int64_t i = 0; i < cap; i++) {
    assert(csp_grunq_try_pop(grunq, &proc));
    assert((int64_t)proc == i);
  }
  assert(!csp_grunq_try_pop(grunq, &proc));

  csp_grunq_destroy(grunq);
}

int main(void) {
  test_lrunq();
  test_grunq();
}
