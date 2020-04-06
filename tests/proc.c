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
#include <stdio.h>
#include "../src/proc.c"

_Thread_local csp_core_t *csp_this_core = &(csp_core_t){.pid = 0};

csp_proc_t *main_proc, *proc;

size_t csp_procs_num = 1;
size_t csp_procs_size[] = {4096};

__attribute__((naked)) void yield(csp_proc_t *from, csp_proc_t *to) {
  __asm__ __volatile__(
    csp_proc_save("rdi")
    "push %rbp\n"
    "mov %rsi, %rdi\n"
    "call csp_proc_restore\n"
  );
}

__attribute__((noinline)) void task(void) {
  printf("This is a task.\n");
  yield(proc, main_proc);
}

void test_proc(void) {
  proc = csp_proc_new(0, false);
  main_proc = csp_proc_new(0, false);

  assert(proc->is_new);
  assert(proc->borned_pid == csp_this_core->pid);
  assert(proc->stat == csp_proc_stat_none);
  assert((proc->rbp & 0x0f) == 0);
  assert(proc->nchild == 0);
  assert(proc->parent == NULL);
  assert(proc->pre == NULL);
  assert(proc->next == NULL);

  proc->rsp = proc->rbp - 8;
  *(uint64_t *)(proc->rsp) = (uint64_t)task;
  yield(main_proc, proc);

  printf("This will be printed after the task.\n");
  csp_proc_destroy(proc);
  csp_proc_destroy(main_proc);
}

int main(void) {
  test_proc();
}
