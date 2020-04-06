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

#include <time.h>
#include "common.h"
#include "core.h"

#define csp_core_anchor_load(reg)                                              \
  "mov (%"reg"),     %rbp\n"                                                   \
  "mov 0x08(%"reg"), %rsp\n"                                                   \
  "mov 0x10(%"reg"), %rax\n"                                                   \
  "mov %rax,         (%rsp)\n"                                                 \
  "mov 0x18(%"reg"), %rbx\n"                                                   \

extern csp_proc_t *csp_sched_get(csp_core_t *this_core);
extern void csp_sched_put_proc(csp_proc_t *proc);
extern bool csp_core_pools_get(size_t pid, csp_core_t **core);
extern void csp_core_pools_put(csp_core_t *core);

_Thread_local csp_core_t *csp_this_core;

csp_core_t *csp_core_new(size_t pid, csp_lrunq_t *lrunq, csp_grunq_t *grunq) {
  csp_core_t *core = (csp_core_t *)malloc(sizeof(csp_core_t));
  if (core == NULL) {
    return NULL;
  }

  core->pid = pid;
  core->lrunq = lrunq;
  core->grunq = grunq;
  core->running = NULL;

  csp_core_state_set(core, csp_core_state_inited);
  pthread_cond_init(&core->cond, NULL);
  pthread_mutex_init(&core->mutex, NULL);
  csp_cond_init(&core->pcond);

  return core;
}

__attribute__((naked,used)) static void csp_core_anchor_save(void *anchor) {
  __asm__ __volatile__(
    "mov %rbp,   (%rdi)\n"
    "mov %rsp,   0x08(%rdi)\n"
    "mov (%rsp), %rax\n"
    "mov %rax,   0x10(%rdi)\n"
    "mov %rbx,   0x18(%rdi)\n"
    "retq\n"
  );
}

__attribute__((naked)) static void csp_core_anchor_restore(void *anchor) {
  __asm__ __volatile__(
    csp_core_anchor_load("rdi")
    "retq\n"
  );
}

__attribute__((noinline)) void *csp_core_run(void *data) {
  csp_core_t *this_core = (csp_core_t *)data;
  csp_core_state_set(this_core, csp_core_state_running);
  csp_this_core = this_core;

  __asm__ __volatile__(
    /* Save variable this_core to rbx. */
    "mov %0, %%rbx\n"

    /* When a process is finished, we should jump here. So we should mark it
    * first. */
    "mov %%rbx, %%rdi\n"
    "call csp_core_anchor_save@plt\n"

    /* Schedue to get the next process. */
    "mov %%rbx, %%rdi\n"
    "call csp_sched_get@plt\n"

    /* Set the process to this_core->running. */
    "mov %%rax, 0x20(%%rbx)\n"

    /* Run the process. */
    "mov %%rax, %%rdi\n"
    "call csp_proc_restore@plt\n"

    ::"r"(this_core) :"rdi", "rbx", "memory"
  );

  return NULL;
}

/* Initialize the main core(running on the main thread). */
void csp_core_init_main(csp_core_t *core) {
  core->tid = pthread_self();
  csp_this_core = core;

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(0, &cpuset);
  pthread_setaffinity_np(core->tid, sizeof(cpuset), &cpuset);
}

/* Start the main core(running on the main thread). */
__attribute__((noinline,used)) void csp_core_start_main(void) {
  csp_core_run(csp_this_core);
}

bool csp_core_start(csp_core_t *core) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core->pid, &cpuset);

  pthread_attr_t attr;
  if (pthread_attr_init(&attr) != 0 ||
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset) != 0 ||
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0 ||
    pthread_create(&core->tid, &attr, csp_core_run, core) != 0) {
    return false;
  }
  pthread_attr_destroy(&attr);
  return true;
}

__attribute__((naked)) void csp_core_yield(csp_proc_t *proc, void *anchor) {
  __asm__ __volatile__(
    csp_proc_save("rdi")
    "push %rbp\n"
    "mov %rsi, %rdi\n"
    "call csp_core_anchor_restore\n"
  );
}

bool csp_core_block_prologue(csp_core_t *this_core) {
  csp_core_t *next;
  if (!csp_core_pools_get(this_core->pid, &next)) {
    return false;
  }

  if (csp_likely(csp_core_state_get(next) != csp_core_state_inited)) {
    csp_core_wakeup(next);
  } else if (!csp_core_start(next)) {
    csp_core_pools_put(next);
    return false;
  }
  return true;
}

__attribute__((used))
static void csp_core_block_epilogue_inner(csp_core_t *this_core) {
  while (!csp_grunq_try_push(this_core->grunq, this_core->running));
  this_core->running = NULL;

  pthread_mutex_lock(&this_core->mutex);
  csp_core_pools_put(this_core);
  pthread_cond_wait(&this_core->cond, &this_core->mutex);
  pthread_mutex_unlock(&this_core->mutex);

  /* Re-schedule finally. */
  csp_core_anchor_restore(&this_core->anchor);
  __builtin_unreachable();
}

__attribute__((naked))
void csp_core_block_epilogue(csp_core_t *core, csp_proc_t *proc) {
  __asm__ __volatile__(
    csp_proc_save("rsi")
    "push %rbp\n"
    "call csp_core_block_epilogue_inner@plt\n"
  );
}

__attribute__((naked))
static void csp_core_proc_exit_inner(csp_proc_t *proc, void *anchor) {
  __asm__ __volatile__(
    /* We must change the stack first, otherwise the stack may be used by other
     * new process immediately after we put it in the memory pool and we are
     * still do some cleaning work like `csp_proc_destroy` on that process
     * stack. */
    csp_core_anchor_load("rsi")
    "call csp_proc_destroy@plt\n"
    "retq\n"
  );
}

/* Called when current process exits. It will clean the process resource and
 * then re-schedule.*/
void csp_core_proc_exit(void) {
  csp_proc_t *running = csp_this_core->running, *parent = running->parent;
  if (parent != NULL && csp_proc_nchild_decr(parent) == 0x01) {
    csp_sched_put_proc(parent);
  }
  csp_this_core->running = NULL;
  csp_core_proc_exit_inner(running, &csp_this_core->anchor);
}

/* Called when current process exits and we want another specified process to
 * run afterwards.
 *
 * NOTE: The current process should not be waited by other processes. */
__attribute__((noreturn)) void csp_core_proc_exit_and_run(csp_proc_t *to_run) {
  csp_core_t *this_core = csp_this_core;
  csp_proc_t *running = this_core->running;
  this_core->running = to_run;

  __asm__ __volatile__ (
    "mov %0, %%r12\n"

    /* Switch to the system thread stack. */
    "mov %1, %%rbp\n"
    "mov %2, %%rsp\n"

    "call csp_proc_destroy@plt\n"
    "mov %%r12, %%rdi\n"
    "call csp_proc_restore@plt\n"
    :
    :"m"(to_run), "r"(this_core->anchor.rbp), "r"(this_core->anchor.rsp),
     "D"(running)
    :"rbp", "rsp", "r12", "memory"
  );
  __builtin_unreachable();
}

void csp_core_destroy(csp_core_t *core) {
  if (core != NULL) {
    pthread_mutex_destroy(&core->mutex);
    pthread_cond_destroy(&core->cond);
    free(core);
  }
}
