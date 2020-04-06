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

#ifndef LIBCSP_CHAN_H
#define LIBCSP_CHAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rbq.h"

#define csp_chan_t(I)                     csp_chan_t_ ## I
#define csp_chan_new(I)                   csp_chan_new_ ## I
#define csp_chan_try_push(c, item)        ((c)->try_push((c)->rbq, (item)))
#define csp_chan_push(c, item)            ((c)->push((c)->rbq, (item)))
#define csp_chan_try_pop(c, item)         ((c)->try_pop((c)->rbq, (item)))
#define csp_chan_pop(c, item)             ((c)->pop((c)->rbq, (item)))
#define csp_chan_try_pushm(c, items, n)   ((c)->try_pushm((c)->rbq, (items), n))
#define csp_chan_pushm(c, items, n)       ((c)->pushm((c)->rbq, (items), n))
#define csp_chan_try_popm(c, items, n)    ((c)->try_popm((c)->rbq, (items), n))
#define csp_chan_popm(c, items, n)        ((c)->popm((c)->rbq, (items), n))
#define csp_chan_destroy(c)                                                    \
  do { (c)->destroy((c)->rbq); free(c); } while (0)                            \

#define csp_chan_declare(K, T, I)                                              \
  csp_ ## K ## rbq_declare(T, I);                                              \
  typedef struct {                                                             \
    void *rbq;                                                                 \
    bool (*try_push)(void *rbq, T item);                                       \
    bool (*try_pushm)(void *rbq, T *items, size_t n);                          \
    bool (*try_pop)(void *rbq, T *item);                                       \
    size_t (*try_popm)(void *rbq, T *, size_t n);                              \
    void (*push)(void *rbq, T item);                                           \
    void (*pushm)(void *rbq, T *item, size_t n);                               \
    void (*pop)(void *rbq, T *item);                                           \
    void (*popm)(void *rbq, T *item, size_t n);                                \
    void (*destroy)(void *rbq);                                                \
  } csp_chan_t(I);                                                             \
  csp_chan_t(I) *csp_chan_new(I)(size_t cap_exp);                              \

#define csp_chan_define(K, T, I)                                               \
  csp_ ## K ## rbq_define(T, I);                                               \
  csp_chan_t(I) *csp_chan_new(I)(size_t cap_exp) {                             \
    csp_chan_t(I) *chan = (csp_chan_t(I) *)malloc(sizeof(csp_chan_t(I)));      \
    if (chan == NULL) {                                                        \
      return NULL;                                                             \
    }                                                                          \
    chan->rbq = csp_ ## K ## rbq_new(I)(cap_exp);                              \
    if (chan->rbq == NULL) {                                                   \
      free(chan);                                                              \
      return NULL;                                                             \
    }                                                                          \
    chan->try_push  = csp_ ## K ## rbq_try_push(I);                            \
    chan->try_pushm = csp_ ## K ## rbq_try_pushm(I);                           \
    chan->try_pop   = csp_ ## K ## rbq_try_pop(I);                             \
    chan->try_popm  = csp_ ## K ## rbq_try_popm(I);                            \
    chan->push      = csp_ ## K ## rbq_push(I);                                \
    chan->pushm     = csp_ ## K ## rbq_pushm(I);                               \
    chan->pop       = csp_ ## K ## rbq_pop(I);                                 \
    chan->popm      = csp_ ## K ## rbq_popm(I);                                \
    chan->destroy   = csp_ ## K ## rbq_destroy(I);                             \
    return chan;                                                               \
  }                                                                            \

#ifdef __cplusplus
}
#endif

#endif
