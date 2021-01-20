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

#include <stdlib.h>
#include "core.h"
#include "corepool.h"

extern int csp_sched_np;
extern size_t csp_max_threads;
extern size_t csp_max_procs_hint;

extern csp_core_t *csp_core_new(
  size_t pid, csp_lrunq_t *lrunq, csp_grunq_t *grunq
);
extern void csp_core_destroy(csp_core_t *core);
static void csp_core_pool_destroy(csp_core_pool_t *pool);

csp_core_pools_t csp_core_pools;

static csp_core_pool_t *
csp_core_pool_new(int pid, size_t grunq_cap_exp, size_t cores_per_cpu) {
  csp_core_pool_t *pool = (csp_core_pool_t *)calloc(1, sizeof(csp_core_pool_t));
  if (pool == NULL) {
    return NULL;
  }

  pool->lrunq = csp_lrunq_new();
  pool->grunq = csp_grunq_new(grunq_cap_exp);
  pool->cores = (csp_core_t **)malloc(sizeof(csp_core_t *) * cores_per_cpu);
  if (pool->lrunq == NULL || pool->grunq == NULL || pool->cores == NULL) {
    goto failed;
  }

  /* We should fulfill the pool thus we can get cached core when current core
   * blocks. */
  for (size_t i = 0; i < cores_per_cpu; i++) {
    pool->cores[i] = csp_core_new(pid, pool->lrunq, pool->grunq);
    if (pool->cores[i] == NULL) {
      pool->cap = i;
      goto failed;
    }
  }

  pool->cap = pool->top = cores_per_cpu;
  csp_mutex_init(&pool->mutex);
  return pool;

failed:
  csp_core_pool_destroy(pool);
  return pool;
}

static void csp_core_pool_push(csp_core_pool_t *pool, csp_core_t *core) {
  csp_mutex_lock(&pool->mutex);
  pool->cores[pool->top++] = core;
  csp_mutex_unlock(&pool->mutex);
}

static bool csp_core_pool_pop(csp_core_pool_t *pool, csp_core_t **core) {
  csp_mutex_lock(&pool->mutex);
  if (pool->top == 0) {
    csp_mutex_unlock(&pool->mutex);
    return false;
  }
  *core = pool->cores[--pool->top];
  csp_mutex_unlock(&pool->mutex);
  return true;
}

static void csp_core_pool_destroy(csp_core_pool_t *pool) {
  if (pool == NULL) {
    return;
  }
  for (size_t i = 0; i < pool->cap; i++) {
    csp_core_destroy(pool->cores[i]);
  }
  csp_lrunq_destroy(pool->lrunq);
  csp_grunq_destroy(pool->grunq);
  free(pool->cores);
  free(pool);
}

bool csp_core_pools_init(void) {
  csp_core_pools.pools = (csp_core_pool_t **)malloc(
    sizeof(csp_core_pool_t *) * csp_sched_np
  );
  if (csp_core_pools.pools == NULL) {
    return false;
  }

  size_t grunq_cap_exp = csp_exp(csp_max_procs_hint / csp_sched_np);
  size_t cores_per_cpu = (csp_max_threads / csp_sched_np) +
    (!!(csp_max_threads % csp_sched_np));

  for (int i = 0; i < csp_sched_np; i++) {
    csp_core_pools.pools[i] = csp_core_pool_new(
      i, grunq_cap_exp, cores_per_cpu
    );
    if (csp_core_pools.pools[i] == NULL) {
      csp_core_pools.len = i;
      return false;
    }
  }
  csp_core_pools.len = csp_sched_np;
  return true;
}

bool csp_core_pools_get(size_t pid, csp_core_t **core) {
  return csp_core_pool_pop(csp_core_pools.pools[pid], core);
}

void csp_core_pools_put(csp_core_t *core) {
  csp_core_pool_push(csp_core_pools.pools[core->pid], core);
}

void csp_core_pools_destroy() {
  if (csp_core_pools.pools == NULL) {
    return;
  }

  for (size_t i = 0; i < csp_core_pools.len; i++) {
    csp_core_pool_destroy(csp_core_pools.pools[i]);
  }
  free(csp_core_pools.pools);
}
