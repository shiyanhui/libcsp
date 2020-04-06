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

#ifndef LIBCSP_RBQ_H
#define LIBCSP_RBQ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

/*
 * `rbq.h` implements a high performance lock-freed ring buffer queue inspired
 * by Disruptor.
 *
 * It implements five kinds of ring buffer queue. i.e,
 * - rrbq:  Raw                               Ring Buffer Queue.
 * - ssrbq: Single   writer  Single   reader  Ring Buffer Queue.
 * - smrbq: Single   writer  Multiple readers Ring Buffer Queue.
 * - msrbq: Multiple writers Single   reader  Ring Buffer Queue.
 * - mmrbq: Multiple writers Multiple readers Ring Buffer Queue.
 *
 * `rrbq` is just a traditional ring buffer and it's not thread-safe.
 * `ssrbq`, `smrbq`, `msrbq` and `mmrbq` are thread-safe, you can use them in
 * different processes.
 */

#define csp_ssrbq_declare(T, I)     csp_rbq_declare(ss, T, I, s, s)
#define csp_ssrbq_define(T, I)      csp_rbq_define(ss, T, I, s, s)
#define csp_ssrbq_t(I)              csp_rbq_name(ss, t, I)
#define csp_ssrbq_new(I)            csp_rbq_name(ss, new, I)
#define csp_ssrbq_try_push(I)       csp_rbq_name(ss, try_push, I)
#define csp_ssrbq_push(I)           csp_rbq_name(ss, push, I)
#define csp_ssrbq_try_pop(I)        csp_rbq_name(ss, try_pop, I)
#define csp_ssrbq_pop(I)            csp_rbq_name(ss, pop, I)
#define csp_ssrbq_try_pushm(I)      csp_rbq_name(ss, try_pushm, I)
#define csp_ssrbq_pushm(I)          csp_rbq_name(ss, pushm, I)
#define csp_ssrbq_try_popm(I)       csp_rbq_name(ss, try_popm, I)
#define csp_ssrbq_popm(I)           csp_rbq_name(ss, popm, I)
#define csp_ssrbq_destroy(I)        csp_rbq_name(ss, destroy, I)

#define csp_smrbq_declare(T, I)     csp_rbq_declare(sm, T, I, s, m)
#define csp_smrbq_define(T, I)      csp_rbq_define(sm, T, I, s, m)
#define csp_smrbq_t(I)              csp_rbq_name(sm, t, I)
#define csp_smrbq_new(I)            csp_rbq_name(sm, new, I)
#define csp_smrbq_try_push(I)       csp_rbq_name(sm, try_push, I)
#define csp_smrbq_push(I)           csp_rbq_name(sm, push, I)
#define csp_smrbq_try_pop(I)        csp_rbq_name(sm, try_pop, I)
#define csp_smrbq_pop(I)            csp_rbq_name(sm, pop, I)
#define csp_smrbq_try_pushm(I)      csp_rbq_name(sm, try_pushm, I)
#define csp_smrbq_pushm(I)          csp_rbq_name(sm, pushm, I)
#define csp_smrbq_try_popm(I)       csp_rbq_name(sm, try_popm, I)
#define csp_smrbq_popm(I)           csp_rbq_name(sm, popm, I)
#define csp_smrbq_destroy(I)        csp_rbq_name(sm, destroy, I)

#define csp_msrbq_declare(T, I)     csp_rbq_declare(ms, T, I, m, s)
#define csp_msrbq_define(T, I)      csp_rbq_define(ms, T, I, m, s)
#define csp_msrbq_t(I)              csp_rbq_name(ms, t, I)
#define csp_msrbq_new(I)            csp_rbq_name(ms, new, I)
#define csp_msrbq_try_push(I)       csp_rbq_name(ms, try_push, I)
#define csp_msrbq_push(I)           csp_rbq_name(ms, push, I)
#define csp_msrbq_try_pop(I)        csp_rbq_name(ms, try_pop, I)
#define csp_msrbq_pop(I)            csp_rbq_name(ms, pop, I)
#define csp_msrbq_try_pushm(I)      csp_rbq_name(ms, try_pushm, I)
#define csp_msrbq_pushm(I)          csp_rbq_name(ms, pushm, I)
#define csp_msrbq_try_popm(I)       csp_rbq_name(ms, try_popm, I)
#define csp_msrbq_popm(I)           csp_rbq_name(ms, popm, I)
#define csp_msrbq_destroy(I)        csp_rbq_name(ms, destroy, I)

#define csp_mmrbq_declare(T, I)     csp_rbq_declare(mm, T, I, m, m)
#define csp_mmrbq_define(T, I)      csp_rbq_define(mm, T, I, m, m)
#define csp_mmrbq_t(I)              csp_rbq_name(mm, t, I)
#define csp_mmrbq_new(I)            csp_rbq_name(mm, new, I)
#define csp_mmrbq_try_push(I)       csp_rbq_name(mm, try_push, I)
#define csp_mmrbq_push(I)           csp_rbq_name(mm, push, I)
#define csp_mmrbq_try_pop(I)        csp_rbq_name(mm, try_pop, I)
#define csp_mmrbq_pop(I)            csp_rbq_name(mm, pop, I)
#define csp_mmrbq_try_pushm(I)      csp_rbq_name(mm, try_pushm, I)
#define csp_mmrbq_pushm(I)          csp_rbq_name(mm, pushm, I)
#define csp_mmrbq_try_popm(I)       csp_rbq_name(mm, try_popm, I)
#define csp_mmrbq_popm(I)           csp_rbq_name(mm, popm, I)
#define csp_mmrbq_destroy(I)        csp_rbq_name(mm, destroy, I)

#define csp_rrbq_declare(T, I)      csp_rrbq_declare_inner(T, I)
#define csp_rrbq_define(T, I)       csp_rrbq_define_inner(T, I)
#define csp_rrbq_t(I)               csp_rbq_name(r, t, I)
#define csp_rrbq_new(I)             csp_rbq_name(r, new, I)
#define csp_rrbq_len(I)             csp_rrbq_len_inner
#define csp_rrbq_try_push(I)        csp_rbq_name(r, try_push, I)
#define csp_rrbq_try_push_front(I)  csp_rbq_name(r, try_push_front, I)
#define csp_rrbq_try_pop(I)         csp_rbq_name(r, try_pop, I)
#define csp_rrbq_try_grow(I)        csp_rbq_name(r, try_grow, I)
#define csp_rrbq_destroy(I)         csp_rbq_name(r, destroy, I)

#define csp_rbq_cap(rbq)            ((rbq)->cap)

/*-------------------------------- csp_rbq_seq -------------------------------*/

typedef char csp_rbq_padding_t[56];

typedef struct {
  csp_rbq_padding_t _;
  atomic_uint_fast64_t v;
} csp_rbq_seq_t;

#define csp_rbq_seq_init(seq, val)       do { (seq).v = (val); } while (0)
#define csp_rbq_seq_get(seq)             atomic_load(&(seq).v)
#define csp_rbq_seq_set(seq, val)        atomic_store(&(seq).v, (val))
#define csp_rbq_seq_cas(seq, oval, nval)                                       \
  atomic_compare_exchange_weak(&(seq).v, &(oval), (nval))                      \

/*-------------------------------- csp_rbq_sptr ------------------------------*/

typedef struct {
  uint_fast64_t next;
  csp_rbq_seq_t barr;
  csp_rbq_padding_t _;
} csp_rbq_sptr_t;

#define csp_rbq_sptr_init(ptr, cap)                                            \
  ({ (ptr).next = 0; csp_rbq_seq_init((ptr).barr, 0); true; })
#define csp_rbq_sptr_mark_avail(ptr, seqv, mask)                               \
  csp_rbq_sptr_barr_set(ptr, csp_rbq_sptr_next_get(ptr))
#define csp_rbq_sptr_markm_avail(ptr, start, end, mask)                        \
  csp_rbq_sptr_barr_set(ptr, csp_rbq_sptr_next_get(ptr))
#define csp_rbq_sptr_next_rsv(ptr, curr, n)                                    \
  ({ csp_rbq_sptr_next_set((ptr), (curr) + (n)); true; })
#define csp_rbq_sptr_next_get(ptr)        ((ptr).next)
#define csp_rbq_sptr_next_set(ptr, val)   do { (ptr).next = (val); } while (0)
#define csp_rbq_sptr_barr_get(ptr)        csp_rbq_seq_get((ptr).barr)
#define csp_rbq_sptr_barr_set(ptr, val)   csp_rbq_seq_set((ptr).barr, (val))
#define csp_rbq_sptr_barr_update(ptr, _)  csp_rbq_sptr_barr_get(ptr)
#define csp_rbq_sptr_destroy(ptr)

/*-------------------------------- csp_rbq_mptr ------------------------------*/

typedef struct {
  csp_rbq_seq_t next, barr;
  csp_rbq_padding_t _;
  csp_rbq_seq_t *stats;
} csp_rbq_mptr_t;

#define csp_rbq_mptr_init(ptr, cap) ({                                         \
  bool ok = false;                                                             \
  csp_rbq_seq_init((ptr).next, 0);                                             \
  csp_rbq_seq_init((ptr).barr, 0);                                             \
  /* The last one is for padding. */                                           \
  (ptr).stats = (csp_rbq_seq_t *)malloc(sizeof(csp_rbq_seq_t) * ((cap) + 1));  \
  if ((ptr).stats != NULL) {                                                   \
    for (size_t i = 0; i < cap; i++) {                                         \
      csp_rbq_seq_init((ptr).stats[i], -1);                                    \
    }                                                                          \
    ok = true;                                                                 \
  }                                                                            \
  ok;                                                                          \
})
#define csp_rbq_mptr_is_avail(ptr, seqv, mask)                                 \
  (csp_rbq_seq_get((ptr).stats[(seqv) & (mask)]) == (seqv))
#define csp_rbq_mptr_mark_avail(ptr, seqv, mask)                               \
  do { csp_rbq_seq_set((ptr).stats[(seqv) & (mask)], (seqv)); } while (0)
/* The range is [start, end) with end excluded. */
#define csp_rbq_mptr_markm_avail(ptr, start, end, mask) do {                   \
  for (uint64_t i = start; i < end; i++) {                                     \
    csp_rbq_mptr_mark_avail(ptr, i, mask);                                     \
  }                                                                            \
} while (0)
#define csp_rbq_mptr_next_rsv(ptr, curr, n)                                    \
  csp_rbq_seq_cas((ptr).next, (curr), (curr) + (n))
#define csp_rbq_mptr_next_get(ptr)       csp_rbq_seq_get((ptr).next)
#define csp_rbq_mptr_next_set(ptr, val)  csp_rbq_seq_set((ptr).next, val)
#define csp_rbq_mptr_barr_get(ptr)       csp_rbq_seq_get((ptr).barr)
#define csp_rbq_mptr_barr_set(ptr, val)  csp_rbq_seq_set((ptr).barr, val)
/* Update the barrier and returns the newly updated barrier. */
#define csp_rbq_mptr_barr_update(ptr, mask) ({                                 \
  uint_fast64_t curr = csp_rbq_mptr_barr_get(ptr), barr = curr;                \
  while (csp_rbq_mptr_is_avail((ptr), barr, (mask))) {                         \
    barr++;                                                                    \
  }                                                                            \
  if (curr != barr) {                                                          \
    if (csp_rbq_seq_cas((ptr).barr, curr, barr)) {                             \
      curr = barr;                                                             \
    } else {                                                                   \
      curr = csp_rbq_mptr_barr_get(ptr);                                       \
    }                                                                          \
  }                                                                            \
  curr;                                                                        \
})
#define csp_rbq_mptr_destroy(ptr)        do { free((ptr).stats); } while (0)

/*-------------------------------- csp_rbq_items -----------------------------*/

#define csp_rbq_items_get(rbq, seqv, itemp)                                    \
  do { *(itemp) = (rbq)->items[(seqv) & (rbq)->mask]; } while (0)
#define csp_rbq_items_set(rbq, seqv, item)                                     \
  do { (rbq)->items[(seqv) & (rbq)->mask] = (item); } while (0)
#define csp_rbq_items_getm(rbq, start, dest, n, T)                             \
  do {                                                                         \
    size_t i = (start) & (rbq)->mask;                                          \
    if (csp_likely(i + (n) <= (rbq)->cap)) {                                   \
      memcpy((dest), (rbq)->items + i, sizeof(T) * (n));                       \
    } else {                                                                   \
      size_t part = (rbq)->cap - i;                                            \
      memcpy((dest), (rbq)->items + i, sizeof(T) * part);                      \
      memcpy((dest) + part, (rbq)->items, sizeof(T) * ((n) - part));           \
    }                                                                          \
  } while (0)
#define csp_rbq_items_setm(rbq, start, src, n, T)                              \
  do {                                                                         \
    size_t i = (start) & (rbq)->mask;                                          \
    if (csp_likely(i + (n) <= (rbq)->cap)) {                                   \
      memcpy((rbq)->items + i, (src), sizeof(T) * (n));                        \
    } else {                                                                   \
      size_t part = (rbq)->cap - i;                                            \
      memcpy((rbq)->items + i, (src), sizeof(T) * part);                       \
      memcpy((rbq)->items, (src) + part, sizeof(T) * ((n) - part));            \
    }                                                                          \
  } while (0)                                                                  \

/*---------------------- thread-safe rbq implementation ----------------------*/

#define csp_rbq_ptr_name(t, name) csp_rbq_ ## t ## ptr_ ## name
#define csp_rbq_name(t, name, I)                                               \
  csp_ ## t ## rbq_ ## name ## _ ## I                                          \

#define csp_rbq_declare(rbqt, T, I, fast_ptr_t, slow_ptr_t)                    \
  typedef struct {                                                             \
    T *items;                                                                  \
    size_t cap, mask;                                                          \
    csp_rbq_ptr_name(slow_ptr_t, t) slow;                                      \
    csp_rbq_ptr_name(fast_ptr_t, t) fast;                                      \
  } csp_rbq_name(rbqt, t, I);                                                  \
                                                                               \
  csp_rbq_name(rbqt, t, I) *csp_rbq_name(rbqt, new, I)(size_t cap_exp);        \
  bool csp_rbq_name(rbqt, try_push, I)(void *rbq, T item);                     \
  void csp_rbq_name(rbqt, push, I)(void *rbq, T item);                         \
  bool csp_rbq_name(rbqt, try_pop, I)(void *rbq, T *item);                     \
  void csp_rbq_name(rbqt, pop, I)(void *rbq, T *item);                         \
  bool csp_rbq_name(rbqt, try_pushm, I)(void *rbq, T *items, size_t n);        \
  void csp_rbq_name(rbqt, pushm, I)(void *rbq, T *items, size_t n);            \
  size_t csp_rbq_name(rbqt, try_popm, I)(void *rbq, T *items, size_t n);       \
  void csp_rbq_name(rbqt, popm, I)(void *rbq, T *items, size_t n);             \
  void csp_rbq_name(rbqt, destroy, I)(void *rbq);                              \

#define csp_rbq_define(rbqt, T, I, fast_ptr_t, slow_ptr_t)                     \
  csp_rbq_name(rbqt, t, I) *csp_rbq_name(rbqt, new, I)(size_t cap_exp) {       \
    csp_rbq_name(rbqt, t, I) *q = (csp_rbq_name(rbqt, t, I) *)malloc(          \
      sizeof(csp_rbq_name(rbqt, t, I))                                         \
    );                                                                         \
    if (q == NULL) {                                                           \
      return NULL;                                                             \
    }                                                                          \
                                                                               \
    q->cap = 1 << cap_exp;                                                     \
    q->mask = q->cap - 1;                                                      \
                                                                               \
    if (!csp_rbq_ptr_name(slow_ptr_t, init)(q->slow, q->cap)) {                \
      free(q);                                                                 \
      return NULL;                                                             \
    }                                                                          \
                                                                               \
    if (!csp_rbq_ptr_name(fast_ptr_t, init)(q->fast, q->cap)) {                \
      csp_rbq_ptr_name(slow_ptr_t, destroy)(q->slow);                          \
      free(q);                                                                 \
      return NULL;                                                             \
    }                                                                          \
                                                                               \
    q->items = (T *)malloc(sizeof(T) * q->cap);                                \
    if (q->items == NULL) {                                                    \
      csp_rbq_ptr_name(fast_ptr_t, destroy)(q->fast);                          \
      csp_rbq_ptr_name(slow_ptr_t, destroy)(q->slow);                          \
      free(q);                                                                 \
      return NULL;                                                             \
    }                                                                          \
                                                                               \
    return q;                                                                  \
  }                                                                            \
                                                                               \
  bool csp_rbq_name(rbqt, try_push, I)(void *rbq, T item) {                    \
    csp_rbq_name(rbqt, t, I) *q = (csp_rbq_name(rbqt, t, I) *)rbq;             \
                                                                               \
    uint_fast64_t                                                              \
      sbarr = csp_rbq_ptr_name(slow_ptr_t, barr_get)(q->slow),                 \
      fnext = csp_rbq_ptr_name(fast_ptr_t, next_get)(q->fast);                 \
                                                                               \
    if (csp_unlikely(sbarr + q->cap <= fnext)) {                               \
      sbarr = csp_rbq_ptr_name(slow_ptr_t, barr_update)(q->slow, q->mask);     \
      if (csp_unlikely(sbarr + q->cap <= fnext)) {                             \
        return false;                                                          \
      }                                                                        \
    }                                                                          \
                                                                               \
    if (csp_likely(                                                            \
        csp_rbq_ptr_name(fast_ptr_t, next_rsv)(q->fast, fnext, 1))) {          \
      csp_rbq_items_set(q, fnext, item);                                       \
      csp_rbq_ptr_name(fast_ptr_t, mark_avail)(q->fast, fnext, q->mask);       \
      return true;                                                             \
    }                                                                          \
    return false;                                                              \
  }                                                                            \
                                                                               \
  void csp_rbq_name(rbqt, push, I)(void *rbq, T item) {                        \
    csp_rbq_name(rbqt, t, I) *q = (csp_rbq_name(rbqt, t, I) *)rbq;             \
                                                                               \
    while (true) {                                                             \
      uint_fast64_t                                                            \
        sbarr = csp_rbq_ptr_name(slow_ptr_t, barr_get)(q->slow),               \
        fnext = csp_rbq_ptr_name(fast_ptr_t, next_get)(q->fast);               \
                                                                               \
      if (csp_unlikely(sbarr + q->cap <= fnext)) {                             \
        sbarr = csp_rbq_ptr_name(slow_ptr_t, barr_update)(q->slow, q->mask);   \
        if (csp_likely(sbarr + q->cap > fnext)) {                              \
          goto push_item;                                                      \
        }                                                                      \
        csp_sched_yield();                                                     \
        continue;                                                              \
      }                                                                        \
                                                                               \
    push_item:                                                                 \
      if (csp_likely                                                           \
          (csp_rbq_ptr_name(fast_ptr_t, next_rsv)(q->fast, fnext, 1))) {       \
        csp_rbq_items_set(q, fnext, item);                                     \
        csp_rbq_ptr_name(fast_ptr_t, mark_avail)(q->fast, fnext, q->mask);     \
        return;                                                                \
      }                                                                        \
    }                                                                          \
  }                                                                            \
                                                                               \
  bool csp_rbq_name(rbqt, try_pop, I)(void *rbq, T *item) {                    \
    csp_rbq_name(rbqt, t, I) *q = (csp_rbq_name(rbqt, t, I) *)rbq;             \
                                                                               \
    uint_fast64_t                                                              \
      snext = csp_rbq_ptr_name(slow_ptr_t, next_get)(q->slow),                 \
      fbarr = csp_rbq_ptr_name(fast_ptr_t, barr_get)(q->fast);                 \
                                                                               \
    if (csp_unlikely(snext >= fbarr)) {                                        \
      fbarr = csp_rbq_ptr_name(fast_ptr_t, barr_update)(q->fast, q->mask);     \
      if (csp_unlikely(snext >= fbarr)) {                                      \
        return false;                                                          \
      }                                                                        \
    }                                                                          \
                                                                               \
    if (csp_likely(                                                            \
        csp_rbq_ptr_name(slow_ptr_t, next_rsv)(q->slow, snext, 1))) {          \
      csp_rbq_items_get(q, snext, item);                                       \
      csp_rbq_ptr_name(slow_ptr_t, mark_avail)(q->slow, snext, q->mask);       \
      return true;                                                             \
    }                                                                          \
    return false;                                                              \
  }                                                                            \
                                                                               \
  void csp_rbq_name(rbqt, pop, I)(void *rbq, T *item) {                        \
    csp_rbq_name(rbqt, t, I) *q = (csp_rbq_name(rbqt, t, I) *)rbq;             \
                                                                               \
    while (true) {                                                             \
      uint_fast64_t                                                            \
        snext = csp_rbq_ptr_name(slow_ptr_t, next_get)(q->slow),               \
        fbarr = csp_rbq_ptr_name(fast_ptr_t, barr_get)(q->fast);               \
                                                                               \
      if (csp_unlikely(snext >= fbarr)) {                                      \
        fbarr = csp_rbq_ptr_name(fast_ptr_t, barr_update)(q->fast, q->mask);   \
        if (csp_likely(snext < fbarr)) {                                       \
          goto pop_item;                                                       \
        }                                                                      \
        csp_sched_yield();                                                     \
        continue;                                                              \
      }                                                                        \
    pop_item:                                                                  \
      if (csp_likely(                                                          \
          csp_rbq_ptr_name(slow_ptr_t, next_rsv)(q->slow, snext, 1))) {        \
        csp_rbq_items_get(q, snext, item);                                     \
        csp_rbq_ptr_name(slow_ptr_t, mark_avail)(q->slow, snext, q->mask);     \
        return;                                                                \
      }                                                                        \
    }                                                                          \
  }                                                                            \
                                                                               \
  bool csp_rbq_name(rbqt, try_pushm, I)(void *rbq, T *items, size_t n) {       \
    csp_rbq_name(rbqt, t, I) *q = (csp_rbq_name(rbqt, t, I) *)rbq;             \
                                                                               \
    if (csp_likely(n > 1)) {                                                   \
      uint_fast64_t                                                            \
        sbarr = csp_rbq_ptr_name(slow_ptr_t, barr_get)(q->slow),               \
        fnext = csp_rbq_ptr_name(fast_ptr_t, next_get)(q->fast);               \
                                                                               \
      if (csp_unlikely(sbarr + q->cap < fnext + n)) {                          \
        sbarr = csp_rbq_ptr_name(slow_ptr_t, barr_update)(q->slow, q->mask);   \
        if (csp_unlikely(sbarr + q->cap < fnext + n)) {                        \
          return false;                                                        \
        }                                                                      \
      }                                                                        \
                                                                               \
      if (csp_likely(                                                          \
          csp_rbq_ptr_name(fast_ptr_t, next_rsv)(q->fast, fnext, n))) {        \
        csp_rbq_items_setm(q, fnext, items, n, T);                             \
        csp_rbq_ptr_name(fast_ptr_t, markm_avail)(                             \
          q->fast, fnext, fnext + n, q->mask                                   \
        );                                                                     \
        return true;                                                           \
      }                                                                        \
      return false;                                                            \
    }                                                                          \
    return n == 1 ? csp_rbq_name(rbqt, try_push, I)(q, *items) : true;         \
  }                                                                            \
                                                                               \
  void csp_rbq_name(rbqt, pushm, I)(void *rbq, T *items, size_t n) {           \
    csp_rbq_name(rbqt, t, I) *q = (csp_rbq_name(rbqt, t, I) *)rbq;             \
                                                                               \
    if (csp_likely(n > 1)) {                                                   \
      size_t chunk = q->cap;                                                   \
      if (n < chunk) {                                                         \
        chunk = n;                                                             \
      }                                                                        \
                                                                               \
      while (n > 0) {                                                          \
        uint_fast64_t                                                          \
          sbarr = csp_rbq_ptr_name(slow_ptr_t, barr_get)(q->slow),             \
          fnext = csp_rbq_ptr_name(fast_ptr_t, next_get)(q->fast);             \
                                                                               \
        if (csp_unlikely(sbarr + q->cap < fnext + chunk)) {                    \
          sbarr = csp_rbq_ptr_name(slow_ptr_t, barr_update)(q->slow, q->mask); \
          if (csp_likely(sbarr + q->cap >= fnext + chunk)) {                   \
            goto push_items;                                                   \
          }                                                                    \
                                                                               \
          if (chunk != 1) {                                                    \
            chunk >>= 1;                                                       \
          } else {                                                             \
            csp_sched_yield();                                                 \
          }                                                                    \
          continue;                                                            \
        }                                                                      \
                                                                               \
      push_items:                                                              \
        if (csp_likely(                                                        \
            csp_rbq_ptr_name(fast_ptr_t, next_rsv)(q->fast, fnext, chunk))) {  \
          if (csp_likely(chunk > 1)) {                                         \
            csp_rbq_items_setm(q, fnext, items, chunk, T);                     \
            csp_rbq_ptr_name(fast_ptr_t, markm_avail)(                         \
              q->fast, fnext, fnext + chunk, q->mask                           \
            );                                                                 \
          } else {                                                             \
            csp_rbq_items_set(q, fnext, *items);                               \
            csp_rbq_ptr_name(fast_ptr_t, mark_avail)(q->fast, fnext, q->mask); \
          }                                                                    \
                                                                               \
          items += chunk;                                                      \
          n -= chunk;                                                          \
        }                                                                      \
      }                                                                        \
    } else if (csp_likely(n == 1)) {                                           \
      csp_rbq_name(rbqt, push, I)(q, *items);                                  \
    }                                                                          \
  }                                                                            \
                                                                               \
  size_t csp_rbq_name(rbqt, try_popm, I)(void *rbq, T *items, size_t n) {      \
    csp_rbq_name(rbqt, t, I) *q = (csp_rbq_name(rbqt, t, I) *)rbq;             \
                                                                               \
    if (csp_likely(n > 1)) {                                                   \
      uint_fast64_t                                                            \
        snext = csp_rbq_ptr_name(slow_ptr_t, next_get)(q->slow),               \
        fbarr = csp_rbq_ptr_name(fast_ptr_t, barr_get)(q->fast);               \
                                                                               \
      if (csp_unlikely(snext >= fbarr)) {                                      \
        fbarr = csp_rbq_ptr_name(fast_ptr_t, barr_update)(q->fast, q->mask);   \
        if (csp_unlikely(snext >= fbarr)) {                                    \
          return 0;                                                            \
        }                                                                      \
      }                                                                        \
                                                                               \
      size_t len = fbarr - snext;                                              \
      if (n < len) {                                                           \
        len = n;                                                               \
      }                                                                        \
                                                                               \
      if (csp_likely(                                                          \
          csp_rbq_ptr_name(slow_ptr_t, next_rsv)(q->slow, snext, len))) {      \
        if (csp_likely(len > 1)) {                                             \
          csp_rbq_items_getm(q, snext, items, len, T);                         \
          csp_rbq_ptr_name(slow_ptr_t, markm_avail)(                           \
            q->slow, snext, snext + len, q->mask                               \
          );                                                                   \
        } else {                                                               \
          csp_rbq_items_get(q, snext, items);                                  \
          csp_rbq_ptr_name(slow_ptr_t, mark_avail)(q->slow, snext, q->mask);   \
        }                                                                      \
        return len;                                                            \
      }                                                                        \
      return 0;                                                                \
    }                                                                          \
    return csp_likely(n == 1) ? csp_rbq_name(rbqt, try_pop, I)(q, items) : 0;  \
  }                                                                            \
                                                                               \
  void csp_rbq_name(rbqt, popm, I)(void *rbq, T *items, size_t n) {            \
    csp_rbq_name(rbqt, t, I) *q = (csp_rbq_name(rbqt, t, I) *)rbq;             \
                                                                               \
    if (csp_likely(n > 1)) {                                                   \
      while (n > 0) {                                                          \
        uint_fast64_t                                                          \
          snext = csp_rbq_ptr_name(slow_ptr_t, next_get)(q->slow),             \
          fbarr = csp_rbq_ptr_name(fast_ptr_t, barr_get)(q->fast), len;        \
                                                                               \
        if (csp_unlikely(snext >= fbarr)) {                                    \
          fbarr = csp_rbq_ptr_name(fast_ptr_t, barr_update)(q->fast, q->mask); \
          if (csp_likely(snext < fbarr)) {                                     \
            goto pop_items;                                                    \
          }                                                                    \
          csp_sched_yield();                                                   \
          continue;                                                            \
        }                                                                      \
                                                                               \
      pop_items:                                                               \
        len = fbarr - snext;                                                   \
        if (n < len) {                                                         \
          len = n;                                                             \
        }                                                                      \
                                                                               \
        if (csp_likely(                                                        \
            csp_rbq_ptr_name(slow_ptr_t, next_rsv)(q->slow, snext, len))) {    \
          if (csp_likely(len > 1)) {                                           \
            csp_rbq_items_getm(q, snext, items, len, T);                       \
            csp_rbq_ptr_name(slow_ptr_t, markm_avail)(                         \
              q->slow, snext, snext + len, q->mask                             \
            );                                                                 \
          } else {                                                             \
            csp_rbq_items_get(q, snext, items);                                \
            csp_rbq_ptr_name(slow_ptr_t, mark_avail)(q->slow, snext, q->mask); \
          }                                                                    \
                                                                               \
          items += len;                                                        \
          n -= len;                                                            \
        }                                                                      \
      }                                                                        \
    } else if (csp_likely(n == 1)) {                                           \
      csp_rbq_name(rbqt, pop, I)(q, items);                                    \
    }                                                                          \
  }                                                                            \
                                                                               \
  void csp_rbq_name(rbqt, destroy, I)(void *rbq) {                             \
    if (rbq == NULL) { return; }                                               \
    csp_rbq_name(rbqt, t, I) *q = (csp_rbq_name(rbqt, t, I) *)rbq;             \
    csp_rbq_ptr_name(slow_ptr_t, destroy)(q->slow);                            \
    csp_rbq_ptr_name(fast_ptr_t, destroy)(q->fast);                            \
    free(q->items);                                                            \
    free(q);                                                                   \
  }                                                                            \

/*--------------------------- raw rbq implementation -------------------------*/

#define csp_rrbq_declare_inner(T, I)                                           \
  typedef struct {                                                             \
    T *items;                                                                  \
    size_t cap, mask;                                                          \
    uint64_t slow, fast;                                                       \
  } csp_rrbq_t(I);                                                             \
                                                                               \
  csp_rrbq_t(I) *csp_rrbq_new(I)(size_t cap_exp);                              \
  bool csp_rrbq_try_push(I)(csp_rrbq_t(I) *q, T item);                         \
  bool csp_rrbq_try_push_front(I)(csp_rrbq_t(I) *q, T item);                   \
  bool csp_rrbq_try_pop(I)(csp_rrbq_t(I) *q, T *item);                         \
  bool csp_rrbq_try_grow(I)(csp_rrbq_t(I) *q);                                 \
  void csp_rrbq_destroy(I)(csp_rrbq_t(I) *q);                                  \

#define csp_rrbq_define_inner(T, I)                                            \
  csp_rrbq_t(I) *csp_rrbq_new(I)(size_t cap_exp) {                             \
    csp_rrbq_t(I) *q = (csp_rrbq_t(I) *)malloc(sizeof(csp_rrbq_t(I)));         \
    if (q == NULL) {                                                           \
      return NULL;                                                             \
    }                                                                          \
                                                                               \
    q->cap = 1 << cap_exp;                                                     \
    q->mask = q->cap - 1;                                                      \
                                                                               \
    q->items = (T *)malloc(sizeof(T) * q->cap);                                \
    if (q->items == NULL) {                                                    \
      free(q);                                                                 \
      return NULL;                                                             \
    }                                                                          \
                                                                               \
    q->slow = q->fast = 0;                                                     \
    return q;                                                                  \
  }                                                                            \
                                                                               \
  bool csp_rrbq_try_push(I)(csp_rrbq_t(I) *q, T item) {                        \
    if (csp_likely(csp_rrbq_len(I)(q) < q->cap)) {                             \
      csp_rbq_items_set(q, q->fast++, item);                                   \
      return true;                                                             \
    }                                                                          \
    return false;                                                              \
  }                                                                            \
                                                                               \
  bool csp_rrbq_try_push_front(I)(csp_rrbq_t(I) *q, T item) {                  \
    if (csp_likely(csp_rrbq_len(I)(q) < q->cap)) {                             \
      csp_rbq_items_set(q, --q->slow, item);                                   \
      return true;                                                             \
    }                                                                          \
    return false;                                                              \
  }                                                                            \
                                                                               \
  bool csp_rrbq_try_pop(I)(csp_rrbq_t(I) *q, T *item) {                        \
    if (csp_unlikely(csp_rrbq_len(I)(q) > 0)) {                                \
      csp_rbq_items_get(q, q->slow++, item);                                   \
      return true;                                                             \
    }                                                                          \
    return false;                                                              \
  }                                                                            \
                                                                               \
  bool csp_rrbq_try_grow(I)(csp_rrbq_t(I) *q) {                                \
    size_t cap = q->cap << 1;                                                  \
    T *items = (T *)realloc(q->items, sizeof(T) * cap);                        \
    if (items != NULL) {                                                       \
      q->items = items;                                                        \
      q->cap = cap;                                                            \
      return true;                                                             \
    }                                                                          \
    return false;                                                              \
  }                                                                            \
                                                                               \
  void csp_rrbq_destroy(I)(csp_rrbq_t(I) *q) {                                 \
    if (q == NULL) { return; }                                                 \
    free(q->items);                                                            \
    free(q);                                                                   \
  }                                                                            \

#define csp_rrbq_len_inner(q) ((size_t)((q)->fast - (q)->slow))

extern void csp_sched_yield(void);

#ifdef __cplusplus
}
#endif

#endif
