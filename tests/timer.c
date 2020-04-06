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
#include <stdlib.h>
#include "../src/proc.c"
#include "../src/timer.c"

int csp_sched_np = 8;
size_t csp_procs_num = 1;
size_t csp_procs_size[] = {4096};
_Thread_local csp_core_t *csp_this_core = &(csp_core_t){.pid = 0};

void csp_sched_yield(void) {}
void csp_core_proc_exit(void) {}

csp_proc_t *start, *end;

csp_proc_t *get_proc(void) {
  return csp_proc_new(0, false);
}

void put_proc(csp_proc_t *proc) {
  csp_proc_destroy(proc);
}

void test_timer_events(void) {
  csp_timer_heap_t heap;
  assert(csp_timer_heap_init(&heap, 0));

  csp_proc_t *proc1 = get_proc();
  proc1->timer.when = 100;
  csp_timer_heap_put(&heap, proc1);
  assert(heap.len == 1);
  assert(heap.token == 1);
  assert(heap.procs[0] == proc1);
  assert(proc1->timer.idx == 0);
  assert(proc1->timer.token == 0);

  csp_proc_t *proc2 = get_proc();
  proc2->timer.when = 50;
  csp_timer_heap_put(&heap, proc2);
  assert(heap.len == 2);
  assert(heap.token == 2);
  assert(heap.procs[0] == proc2);
  assert(heap.procs[1] == proc1);
  assert(proc1->timer.idx == 1);
  assert(proc1->timer.token == 0);
  assert(proc2->timer.idx == 0);
  assert(proc2->timer.token == 1);

  csp_proc_t *proc3 = get_proc();
  proc3->timer.when = 150;
  csp_timer_heap_put(&heap, proc3);
  assert(heap.len == 3);
  assert(heap.token == 3);
  assert(heap.procs[0] == proc2);
  assert(heap.procs[1] == proc1);
  assert(heap.procs[2] == proc3);
  assert(proc1->timer.idx == 1);
  assert(proc1->timer.token == 0);
  assert(proc2->timer.idx == 0);
  assert(proc2->timer.token == 1);
  assert(proc3->timer.idx == 2);
  assert(proc3->timer.token == 2);

  csp_timer_heap_del(&heap, proc2);
  assert(heap.len == 2);
  assert(heap.token == 3);
  assert(heap.procs[0] == proc1);
  assert(heap.procs[1] == proc3);
  assert(proc1->timer.idx == 0);
  assert(proc1->timer.token == 0);
  assert(proc3->timer.idx == 1);
  assert(proc3->timer.token == 2);

  csp_timer_heap_del(&heap, proc1);
  assert(heap.len == 1);
  assert(heap.token == 3);
  assert(heap.procs[0] == proc3);
  assert(proc3->timer.idx == 0);
  assert(proc3->timer.token == 2);

  csp_timer_heap_del(&heap, proc3);
  assert(heap.len == 0);
  assert(heap.token == 3);

  csp_timer_heap_destroy(&heap);
}

void test_timer_heaps(void) {
  assert(csp_timer_heaps_init());
  csp_timer_heaps_destroy();
}

void test_timer(void) {
  csp_timer_heaps_init();

  csp_proc_t *proc1 = get_proc();
  proc1->timer.when = 0;
  csp_timer_put(0, proc1);
  assert(csp_timer_heaps.heaps[0].len == 1);
  assert(csp_timer_heaps.heaps[0].token == 1);
  assert(csp_timer_heaps.heaps[0].procs[0] == proc1);
  assert(proc1->borned_pid == 0);
  assert(proc1->timer.idx == 0);
  assert(proc1->timer.token == 0);

  csp_proc_t *proc2 = get_proc();
  proc2->timer.when = INT64_MAX;
  csp_timer_put(0, proc2);
  assert(csp_timer_heaps.heaps[0].len == 2);
  assert(csp_timer_heaps.heaps[0].token == 2);
  assert(csp_timer_heaps.heaps[0].procs[0] == proc1);
  assert(proc2->borned_pid == 0);
  assert(proc2->timer.idx == 1);
  assert(proc2->timer.token == 1);

  for (int i = 1; i < csp_sched_np; i++) {
    assert(csp_timer_heap_get(&csp_timer_heaps.heaps[i], &start, &end) == 0);
  }
  assert(csp_timer_heap_get(&csp_timer_heaps.heaps[0], &start, &end) == 1);
  assert(start == end);
  assert(start == proc1);
  assert(csp_timer_heaps.heaps[0].len == 1);
  assert(csp_timer_heaps.heaps[0].token == 2);
  assert(csp_timer_heaps.heaps[0].procs[0] == proc2);
  assert(csp_timer_heap_get(&csp_timer_heaps.heaps[0], &start, &end) == 0);

  put_proc(proc2);
  csp_timer_heaps_destroy();
}

int main(void) {
  test_timer_events();
  test_timer_heaps();
  test_timer();
}
