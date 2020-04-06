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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <assert.h>
#include <stdlib.h>
#include "../src/runq.c"
#include "../src/proc.c"
#include "../src/core.c"
#include "../src/corepool.c"

int csp_sched_np = 1;
size_t csp_max_threads = 1;
size_t csp_max_procs_hint = 100;

size_t csp_procs_num = 1;
size_t csp_procs_size[] = {4096};

void csp_sched_put_proc(csp_proc_t *proc) {}
void csp_sched_yield() {}

csp_proc_t *csp_sched_get(csp_core_t *this_core) {
  return NULL;
}

void test_core_pool(void) {
  size_t stack_cap = 3, pid = 0, lrunq_cap_exp = 3;
  csp_core_t *cores[stack_cap], *core;
  csp_proc_t proc;

  /* Test csp_core_pool_new. */
  csp_core_pool_t *stack = csp_core_pool_new(stack_cap, pid, lrunq_cap_exp);
  assert(stack->top == stack_cap);

  /* Test csp_core_pool_pop. */
  for (int i = 0; i < stack_cap; i++) {
    assert(csp_core_pool_pop(stack, &core));
    assert(stack->top == stack_cap - i - 1);
    cores[i] = core;
  }
  assert(!csp_core_pool_pop(stack, &core));

  /* Test csp_core_pool_push. */
  for (int i = 0; i < stack_cap; i++) {
    csp_core_pool_push(stack, cores[i]);
    assert(stack->cores[i] == cores[i]);
    assert(stack->top == i + 1);
  }
  assert(stack->top == stack_cap);

  csp_core_pool_destroy(stack);
}

int main(void) {
  test_core_pool();
}
