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

#ifndef LIBCSP_RUNQ_H
#define LIBCSP_RUNQ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include "proc.h"
#include "rbq.h"
#include "mutex.h"

#define csp_grunq_t          csp_mmrbq_t(proc)
#define csp_grunq_new        csp_mmrbq_new(proc)
#define csp_grunq_try_push   csp_mmrbq_try_push(proc)
#define csp_grunq_try_pushm  csp_mmrbq_try_pushm(proc)
#define csp_grunq_try_pop    csp_mmrbq_try_pop(proc)
#define csp_grunq_destroy    csp_mmrbq_destroy(proc)

#define csp_lrunq_ok         0
#define csp_lrunq_failed     -1
#define csp_lrunq_missed     1
#define csp_lrunq_len(lrunq) ((lrunq)->len)
#define csp_lrunq_set(lrunq, n, start, end) do {                               \
  (lrunq)->head = (start);                                                     \
  (lrunq)->tail = (end);                                                       \
  (lrunq)->len = (n);                                                          \
} while (0)                                                                    \

csp_mmrbq_declare(csp_proc_t *, proc);

typedef struct {
  csp_proc_t *head, *tail;
  size_t len;
  int64_t poped_times;
} csp_lrunq_t;

csp_lrunq_t *csp_lrunq_new();
void csp_lrunq_push(csp_lrunq_t *lrunq, csp_proc_t *proc);
void csp_lrunq_push_front(csp_lrunq_t *lrunq, csp_proc_t *proc);
int csp_lrunq_try_pop_front(csp_lrunq_t *lrunq, csp_proc_t **proc);
void csp_lrunq_popm_front(csp_lrunq_t *lrunq, size_t n, csp_proc_t **start,
  csp_proc_t **end
);
void csp_lrunq_destroy(csp_lrunq_t *lrunq);

#ifdef __cplusplus
}
#endif

#endif
