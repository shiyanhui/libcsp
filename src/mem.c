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

#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "common.h"
#include "core.h"
#include "rbq.h"
#include "rbtree.h"

/*
 * mem.c implements a virtual memory manager.
 *
 * The x86_64 CPU with 4-level paging uses 48 bits(16:9:9:9:9:12). Linux uses
 * the 48th bit to distinguish user space and kernel space. So we have total
 * 47-bits virtual user space to manage.
 *
 * We make the page size the same as the system, i.e. 4KB. And to reduce the
 * meta information, we use the same strategy as the system - paging which has
 * three levels. The first level is cpu_id, which takes 12 bytes, thus each CPU
 * can occupy 64GB and libcsp can support 2048 CPUs at most. The reason why we
 * make each CPU to take a continuous segment memory is that we can use a
 * lock-free algorithm to malloc and free memory in it. The second level is the
 * l1 level(Page Directory) which takes 8 bytes and the third level(Page Table)
 * which takes 16 bytes. The layout is,
 *
 * ← High bits                                                    Low bits →
 * +-----------------------------------------------------------------------+
 * | reversed(16B) | user(1B) | cpu_id(12B) | l1(8B) | l2(16B) | page(12B) |
 * +-----------------------------------------------------------------------+
 */

#define csp_mem_heap_size_exp     36
#define csp_mem_heap_size         (1L << csp_mem_heap_size_exp)
#define csp_mem_heap_offset(heap, addr) ((uintptr_t)(addr) - (heap)->start)
#define csp_mem_heap_init_l1(heap, l1) ({                                      \
  (heap)->metas[l1] = csp_mem_meta_new(l1);                                    \
  (heap)->mailboxes[l1] = csp_msrbq_new(obj)(csp_mem_meta_l2_num_exp);         \
  ((heap)->metas[l1] != NULL && (heap)->mailboxes[l1] != NULL);                \
})
#define csp_mem_heap_destroy_l1(heap, l1) do {                                 \
  if ((heap)->metas[l1] != NULL) {                                             \
    csp_mem_meta_destroy((heap)->metas[l1]);                                   \
  }                                                                            \
  if ((heap)->mailboxes[l1] != NULL) {                                         \
    csp_msrbq_destroy(obj)((heap)->mailboxes[l1]);                             \
  }                                                                            \
} while (0)

#define csp_mem_arena_size_exp    24
#define csp_mem_arena_size        (1 << csp_mem_arena_size_exp)
#define csp_mem_arena_npages      (csp_mem_arena_size / csp_mem_page_size)
#define csp_mem_arena_new(heap) ({                                             \
  void *arena_;                                                                \
  do {                                                                         \
    (heap)->curr += csp_mem_arena_size;                                        \
    if ((heap)->curr >= (heap)->end) {                                         \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
    arena_ = mmap((void *)(heap)->curr, csp_mem_arena_size,                    \
      PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0         \
    );                                                                         \
  } while (arena_ == MAP_FAILED);                                              \
                                                                               \
  int32_t l1_ = csp_mem_meta_l1_by_addr(heap, arena_);                         \
  if ((heap)->metas[l1_] == NULL && !csp_mem_heap_init_l1(heap, l1_)) {        \
    munmap(arena_, csp_mem_arena_size);                                        \
    exit(EXIT_FAILURE);                                                        \
  }                                                                            \
                                                                               \
  csp_mem_arena_link_t *link_ = (csp_mem_arena_link_t *)malloc(                \
    sizeof(csp_mem_arena_link_t)                                               \
  );                                                                           \
  if (link_ == NULL) {                                                         \
    munmap(arena_, csp_mem_arena_size);                                        \
    exit(EXIT_FAILURE);                                                        \
  }                                                                            \
  link_->addr = arena_;                                                        \
  link_->next = (heap)->arenas;                                                \
  (heap)->arenas = link_;                                                      \
                                                                               \
  arena_;                                                                      \
})                                                                             \

#define csp_mem_page_size_exp      12
#define csp_mem_page_size          (1 << csp_mem_page_size_exp)

#define csp_mem_meta_l1_num_exp    8
#define csp_mem_meta_l1_num        (1 << csp_mem_meta_l1_num_exp)
#define csp_mem_meta_l1_size       (csp_mem_heap_size / csp_mem_meta_l1_num)
#define csp_mem_meta_l1_size_mask  (csp_mem_meta_l1_size - 1)
#define csp_mem_meta_l1_by_index(index) ((uint32_t)((index)[0]))
#define csp_mem_meta_l1_by_addr(heap, addr)                                    \
  (csp_mem_heap_offset(heap, addr) / csp_mem_meta_l1_size)

#define csp_mem_meta_l2_num_exp                                                \
  (csp_mem_heap_size_exp - csp_mem_meta_l1_num_exp - csp_mem_page_size_exp)
#define csp_mem_meta_l2_num        (1 << csp_mem_meta_l2_num_exp)
#define csp_mem_meta_l2_num_mask   (csp_mem_meta_l2_num - 1)
#define csp_mem_meta_l2_by_index(index)                                        \
  ((((uint32_t)((index)[1])) << 8) | ((uint32_t)((index)[2])))
#define csp_mem_meta_l2_by_addr(heap, addr)                                    \
  ((csp_mem_heap_offset(heap, addr) & csp_mem_meta_l1_size_mask) /             \
    csp_mem_page_size)

#define csp_mem_meta_l1l2_to_addr(heap, l1, l2) ((uintptr_t)                   \
  ((l1) * csp_mem_meta_l1_size + (l2) * csp_mem_page_size + (heap)->start))    \

#define csp_mem_meta_new(l1) ({                                                \
  csp_mem_meta_t *meta_ = (csp_mem_meta_t *)calloc(1, sizeof(csp_mem_meta_t)); \
  if (meta_ != NULL) {                                                         \
    for (int32_t l2_ = 0; l2_ < csp_mem_meta_l2_num; l2_++) {                  \
      csp_mem_meta_index_set_by_l1l2(meta_->spans[l2_].index, l1, l2_);        \
    }                                                                          \
  }                                                                            \
  meta_;                                                                       \
})
#define csp_mem_meta_taken_bit_by_index(heap, index) ({                        \
  int32_t l1_ = csp_mem_meta_l1_by_index(index);                               \
  int32_t l2_ = csp_mem_meta_l2_by_index(index);                               \
  csp_mem_meta_taken_bit(heap, l1_, l2_);                                      \
})
#define csp_mem_meta_taken_bit(heap, l1, l2) ((                                \
  ((heap)->metas[l1])->taken_bits[(l2) >> 3] >>                                \
  (0x07 - (((uint8_t)l2) & 0x07))                                              \
) & 0x01)
#define csp_mem_meta_taken_bit_set(heap, l1, l2) do {                          \
  ((heap)->metas[l1])->taken_bits[(l2) >> 3] |=                                \
    (uint8_t)0x01 << (0x07 - (((uint8_t)l2) & 0x07));                          \
} while (0)
#define csp_mem_meta_taken_bit_clear(heap, l1, l2) do {                        \
  ((heap)->metas[l1])->taken_bits[(l2) >> 3] &=                                \
    ~((uint8_t)0x01 << (0x07 - (((uint8_t)l2) & 0x07)));;                      \
} while (0)
#define csp_mem_meta_span_by_l1l2(heap, l1, l2)                                \
  (&((heap)->metas[l1]->spans[l2]))
#define csp_mem_meta_span_by_addr(heap, addr) ({                               \
  int32_t l1_ = csp_mem_meta_l1_by_addr(heap, addr);                           \
  int32_t l2_ = csp_mem_meta_l2_by_addr(heap, addr);                           \
  csp_mem_meta_span_by_l1l2(heap, l1_, l2_);                                   \
})
#define csp_mem_meta_span_by_index(heap, index) ({                             \
  csp_mem_span_t *span_ = NULL;                                                \
  if (!csp_mem_meta_index_is_zero(index)) {                                    \
    int32_t l1_ = csp_mem_meta_l1_by_index(index);                             \
    int32_t l2_ = csp_mem_meta_l2_by_index(index);                             \
    span_ = csp_mem_meta_span_by_l1l2(heap, l1_, l2_);                         \
  }                                                                            \
  span_;                                                                       \
})
#define csp_mem_meta_destroy(meta) free(meta)

#define csp_mem_span_npages_get(span)                                          \
  ((((uint32_t)((span)->npages[0])) << 16) |                                   \
   (((uint32_t)((span)->npages[1])) << 8)  |                                   \
   ((uint32_t)((span)->npages[2])))
#define csp_mem_span_npages_set(span, n) do {                                  \
   (span)->npages[0] = (uint8_t)((n) >> 16);                                   \
   (span)->npages[1] = (uint8_t)((n) >> 8);                                    \
   (span)->npages[2] = (uint8_t)(n);                                           \
} while (0)
#define csp_mem_span_is_free(heap, span)                                       \
  (((span) != NULL) && !csp_mem_meta_taken_bit_by_index((heap), (span)->index))
#define csp_mem_span_remove(heap, span, total) ({                              \
  int npages_ = csp_mem_span_npages_get(span);                                 \
  csp_rbtree_node_t *node_ = csp_mem_tree_node_get(heap, npages_);             \
  (total) += npages_;                                                          \
  csp_mem_tree_node_del_span(heap, node_, span);                               \
})                                                                             \

#define csp_mem_meta_index_set(index, value) do {                              \
  (index)[0] = (value)[0];                                                     \
  (index)[1] = (value)[1];                                                     \
  (index)[2] = (value)[2];                                                     \
} while (0)
#define csp_mem_meta_index_set_by_l1l2(index, l1, l2) do {                     \
  (index)[0] = (int8_t)l1;                                                     \
  (index)[1] = (int16_t)l2 >> 8;                                               \
  (index)[2] = (int8_t)l2;                                                     \
} while (0)
#define csp_mem_meta_index_set_zero(index) do {                                \
  (index)[0] = (index)[1] = (index)[2] = 0;                                    \
} while (0)
#define csp_mem_meta_index_is_zero(index)                                      \
  (!((index)[0] | (index)[1] | (index)[2]))

#define csp_mem_tree_node_num (csp_mem_arena_size / csp_mem_page_size)
#define csp_mem_tree_node_cache_get(heap, npages) ((heap)->cache_nodes[npages])
#define csp_mem_tree_node_cache_set(heap, npages, node)                        \
  (heap)->cache_nodes[npages] = (node)
#define csp_mem_tree_node_get_gte(heap, npages)                                \
  csp_mem_tree_node_get_by_func(heap, npages, csp_rbtree_find_gte)
#define csp_mem_tree_node_get(heap, npages)                                    \
  csp_mem_tree_node_get_by_func(heap, npages, csp_rbtree_find)
#define csp_mem_tree_node_get_by_func(heap, npages, func) ({                   \
  csp_rbtree_node_t *node_ = csp_mem_tree_node_cache_get(heap, npages);        \
  if (node_ == NULL) {                                                         \
    node_ = (func)((heap)->tree, (npages));                                    \
    if (node_ != NULL) {                                                       \
      csp_mem_tree_node_cache_set(heap, node_->key, node_);                    \
    }                                                                          \
  }                                                                            \
  node_;                                                                       \
})
#define csp_mem_tree_node_put_span(heap, node, span) do {                      \
  csp_mem_span_t *next_ = (csp_mem_span_t *)((node)->value);                   \
  if (next_ != NULL) {                                                         \
    csp_mem_meta_index_set((span)->fp_next, next_->index);                     \
    csp_mem_meta_index_set(next_->fp_pre, (span)->index);                      \
  } else {                                                                     \
    csp_mem_meta_index_set_zero((span)->fp_next);                              \
  }                                                                            \
  (node)->value = (span);                                                      \
  csp_mem_tree_node_cache_set((heap), (node)->key, (node));                    \
} while (0)
#define csp_mem_tree_node_del_span(heap, node, span) ({                        \
  csp_mem_span_t                                                               \
    *pre_ = csp_mem_meta_span_by_index(heap, (span)->fp_pre),                  \
    *next_ = csp_mem_meta_span_by_index(heap, (span)->fp_next);                \
  if (pre_ != NULL && next_ != NULL) {                                         \
    csp_mem_meta_index_set(pre_->fp_next, next_->index);                       \
    csp_mem_meta_index_set(next_->fp_pre, pre_->index);                        \
    csp_mem_meta_index_set_zero((span)->fp_pre);                               \
    csp_mem_meta_index_set_zero((span)->fp_next);                              \
  } else if (pre_ != NULL) {                                                   \
    csp_mem_meta_index_set_zero(pre_->fp_next);                                \
    csp_mem_meta_index_set_zero((span)->fp_pre);                               \
  } else if (next_ != NULL) {                                                  \
    csp_mem_meta_index_set_zero(next_->fp_pre);                                \
    csp_mem_meta_index_set_zero((span)->fp_next);                              \
    (node)->value = next_;                                                     \
  } else {                                                                     \
    csp_mem_tree_node_cache_set(heap, (node)->key, NULL);                      \
    csp_rbtree_node_t *succ_ = csp_rbtree_delete((heap)->tree, node);          \
    if (succ_ != NULL) {                                                       \
      csp_mem_tree_node_cache_set(heap, succ_->key, succ_);                    \
    }                                                                          \
  }                                                                            \
  next_;                                                                       \
})                                                                             \

extern int csp_sched_np;
extern _Thread_local csp_core_t *csp_this_core;

csp_msrbq_declare(uintptr_t, obj);
csp_msrbq_define(uintptr_t, obj);

typedef uint8_t csp_mem_meta_index_t[3];

typedef struct {
  uint8_t npages[3];
  csp_mem_meta_index_t index, mt_pre, mt_next, fp_pre, fp_next;
} csp_mem_span_t;

typedef struct {
  csp_mem_span_t spans[csp_mem_meta_l2_num];
  uint8_t taken_bits[csp_mem_meta_l2_num / sizeof(uint8_t)];
} csp_mem_meta_t;

typedef struct csp_mem_arena_link_t {
  void *addr;
  struct csp_mem_arena_link_t *next;
} csp_mem_arena_link_t;

typedef struct {
  /* The range of heap memory space is [start, end). */
  uintptr_t start, end;

  /* The pointer approaching to the end. */
  uintptr_t curr;

  /* Store the mmapped arenas. */
  csp_mem_arena_link_t *arenas;

  /* Store the metadata of pages. */
  csp_mem_meta_t *metas[csp_mem_meta_l1_num];

  /* Store the objects returned from other cores. */
  csp_msrbq_t(obj) *mailboxes[csp_mem_meta_l1_num];

  /* Store free pages. The key is the pages number and the value is the free
   * span list */
  csp_rbtree_t *tree;

  /* Cache the tree nodes to speed the searching. */
  csp_rbtree_node_t *cache_nodes[csp_mem_tree_node_num];

  /* Store all nodes in the red-black tree temporarily. */
  csp_rbtree_node_t *all_nodes[csp_mem_tree_node_num];

  /* Store all keys in the red-black tree temporarily. */
  int all_keys[csp_mem_tree_node_num];
} csp_mem_heap_t;

static bool csp_mem_heap_init(csp_mem_heap_t *heap, uintptr_t start) {
  memset(heap->metas, 0, sizeof(heap->metas));
  memset(heap->mailboxes, 0, sizeof(heap->mailboxes));
  memset(heap->cache_nodes, 0, sizeof(heap->cache_nodes));

  heap->arenas = NULL;

  heap->tree = csp_rbtree_new();
  if (heap->tree == NULL) {
    return false;
  }

  heap->start = start;
  heap->end = start + csp_mem_heap_size;

  /* We will add `csp_mem_arena_size` to the `heap->curr` first and then mmap
   * memory from the OS in `csp_mem_arena_new`, so we need to subtract
   * `csp_mem_arena_size` from `start` here. */
  heap->curr = start - csp_mem_arena_size;

  uintptr_t mem = (uintptr_t)csp_mem_arena_new(heap);
  size_t n = csp_mem_arena_npages;

  /* We use [0, 0, 0] to represent NULL for `csp_mem_meta_index_t`. However
   * [0, 0, 0] means the first page of the heap, so we shoud skip it. */
  if (mem == heap->start) {
    mem += csp_mem_page_size;
    n--;
  }

  csp_rbtree_node_t *node = csp_rbtree_insert(heap->tree, n);
  csp_mem_span_t *span = csp_mem_meta_span_by_addr(heap, mem);
  csp_mem_span_npages_set(span, n);
  csp_mem_tree_node_put_span(heap, node, span);

  return true;
}

/* Merge all adjacent spans. */
static void csp_mem_heap_merge(csp_mem_heap_t *heap) {
  csp_rbtree_node_t *node;

  int n = csp_rbtree_all_nodes(heap->tree, heap->all_nodes);
  for (int i = 0; i < n; i++) {
    node = heap->all_nodes[i];
    heap->all_keys[i] = node->key;
    csp_mem_tree_node_cache_set(heap, node->key, node);
  }

  /* We merge the spans in the reversed order of node key(i.e. pages number)
   * thus we won't try to merge the new merged span again. */
  for (int i = n - 1; i >= 0; i--) {
    node = csp_mem_tree_node_cache_get(heap, heap->all_keys[i]);

    /* `NULL` means the node has beed deleted. */
    if (node == NULL) {
      continue;
    }

    csp_mem_span_t *span = (csp_mem_span_t *)node->value;
    while (span != NULL) {
      int total = 0;

      csp_mem_span_t
        *pre = csp_mem_meta_span_by_index(heap, span->mt_pre),
        *next = csp_mem_meta_span_by_index(heap, span->mt_next),
        *start = span, *end = span;

      while (csp_mem_span_is_free(heap, pre)) {
        csp_mem_span_remove(heap, pre, total);
        start = pre;
        pre = csp_mem_meta_span_by_index(heap, pre->mt_pre);
      }

      while (csp_mem_span_is_free(heap, next)) {
        csp_mem_span_remove(heap, next, total);
        end = next;
        next = csp_mem_meta_span_by_index(heap, next->mt_next);
      }

      if (start == end) {
        span = csp_mem_meta_span_by_index(heap, span->fp_next);
        continue;
      }

      span = csp_mem_span_remove(heap, span, total);

      node = csp_rbtree_insert(heap->tree, total);
      csp_mem_span_npages_set(start, total);
      csp_mem_tree_node_put_span(heap, node, start);

      if (next) {
        csp_mem_meta_index_set(start->mt_next, next->index);
        csp_mem_meta_index_set(next->mt_pre, start->index);
      } else {
        csp_mem_meta_index_set_zero(start->mt_next);
      }
    }
  }
}

/* Free a memory object from the heap. */
static void csp_mem_heap_free(csp_mem_heap_t *heap, void *obj) {
  int32_t l1 = csp_mem_meta_l1_by_addr(heap, obj);
  int32_t l2 = csp_mem_meta_l2_by_addr(heap, obj);
  csp_mem_meta_taken_bit_clear(heap, l1, l2);

  csp_mem_span_t *curr = csp_mem_meta_span_by_l1l2(heap, l1, l2);
  csp_rbtree_node_t *node = csp_rbtree_insert(
    heap->tree, csp_mem_span_npages_get(curr)
  );
  csp_mem_tree_node_put_span(heap, node, curr);
}

/* Allocate n pages from the heap. `size` is guaranteed to 4KB alignment.*/
static void *csp_mem_heap_alloc(csp_mem_heap_t *heap, size_t size) {
  /* The max size is csp_mem_arena_size. */
  if (csp_unlikely(size > csp_mem_arena_size)) {
    size = csp_mem_arena_size;
  }

  void *result;
  int npages = size >> csp_mem_page_size_exp;

  csp_rbtree_node_t *node = csp_mem_tree_node_get_gte(heap, npages);
  if (node == NULL) {
    /* Try to collect free pages returned by other prcessors. */
    bool is_freed = false;
    for (int i = 0; i < csp_mem_meta_l1_num; i++) {
      csp_msrbq_t(obj) *mailbox = heap->mailboxes[i];
      if (mailbox == NULL) {
        break;
      }
      size_t n;
      uintptr_t objs[16];
      while ((n = csp_msrbq_try_popm(obj)(mailbox, objs, 16)) > 0) {
        is_freed = true;
        for (size_t j = 0; j < n; j++) {
          csp_mem_heap_free(heap, (void *)objs[j]);
        }
        if (n < 16) {
          break;
        }
      }
    }
    if (is_freed) {
      node = csp_mem_tree_node_get_gte(heap, npages);
    }

    /* Try to merge adjacent free spans. */
    if (node == NULL && npages > 1) {
      csp_mem_heap_merge(heap);
      node = csp_mem_tree_node_get_gte(heap, npages);
    }
  }

  /* Free pages found. */
  if (node != NULL) {
    int key = node->key;
    csp_mem_span_t *span = (csp_mem_span_t *)node->value;

    /* Delete it from the free list. */
    csp_mem_tree_node_del_span(heap, node, span);

    int32_t l1 = csp_mem_meta_l1_by_index(span->index);
    int32_t l2 = csp_mem_meta_l2_by_index(span->index);
    csp_mem_meta_taken_bit_set(heap, l1, l2);

    result = (void *)csp_mem_meta_l1l2_to_addr(heap, l1, l2);

    /* Split the span if it's larger than the request. */
    if (key > npages) {
      csp_mem_span_npages_set(span, npages);

      /* Compute the index of the new span. */
      int32_t new_l1 = l1, new_l2 = l2 + npages, overflow;
      if (new_l2 >= csp_mem_meta_l2_num) {
        overflow = new_l2 - csp_mem_meta_l2_num + 1;

        new_l2 = overflow & csp_mem_meta_l2_num_mask;
        new_l1 += overflow / csp_mem_meta_l2_num + (!!new_l2);

        if (heap->metas[new_l1] == NULL &&
            !csp_mem_heap_init_l1(heap, new_l1)) {
          exit(EXIT_FAILURE);
        }
      }

      /* Get the new span. */
      csp_mem_span_t *new_span = csp_mem_meta_span_by_l1l2(
        heap, new_l1, new_l2
      );
      csp_mem_span_npages_set(new_span, key - npages);
      csp_mem_meta_taken_bit_clear(heap, new_l1, new_l2);

      /* Insert the new span to the metadata list. */
      csp_mem_span_t *next = csp_mem_meta_span_by_index(heap, span->mt_next);
      csp_mem_meta_index_set(new_span->mt_pre, span->index);
      csp_mem_meta_index_set(new_span->mt_next, span->mt_next);
      csp_mem_meta_index_set(span->mt_next, new_span->index);
      if (next != NULL) {
        csp_mem_meta_index_set(next->mt_pre, new_span->index);
      }

      /* Insert the new span to the free pages list. */
      node = csp_rbtree_insert(heap->tree, key - npages);
      csp_mem_tree_node_put_span(heap, node, new_span);
    }

    return result;
  }

  /* Allocate from the OS. */
  result = csp_mem_arena_new(heap);

  /* Initialize the span. */
  int32_t l1 = csp_mem_meta_l1_by_addr(heap, result);
  int32_t l2 = csp_mem_meta_l2_by_addr(heap, result);
  csp_mem_span_t *span = csp_mem_meta_span_by_l1l2(heap, l1, l2);
  csp_mem_span_npages_set(span, npages);
  csp_mem_meta_taken_bit_set(heap, l1, l2);

  /* Split the span. */
  if (size < csp_mem_arena_size) {
    uintptr_t addr = (uintptr_t)result + size;
    csp_mem_span_t *new_span = csp_mem_meta_span_by_addr(heap, addr);
    csp_mem_span_npages_set(new_span, csp_mem_arena_npages - npages);

    /* Link the two parts in metadata list. */
    csp_mem_meta_index_set(span->mt_next, new_span->index);
    csp_mem_meta_index_set(new_span->mt_pre, span->index);

    /* Put to the free pages list. */
    node = csp_rbtree_insert(heap->tree, csp_mem_arena_npages - npages);
    csp_mem_tree_node_put_span(heap, node, new_span);
  }

  return result;
}

static void csp_mem_heap_destroy(csp_mem_heap_t *heap) {
  for (int i = 0; i < csp_mem_meta_l1_num; i++) {
    csp_mem_heap_destroy_l1(heap, i);
  }

  csp_mem_arena_link_t *link = heap->arenas, *next;
  while (link != NULL) {
    next = link->next;
    munmap(link->addr, csp_mem_arena_size);
    free(link);
    link = next;
  }

  csp_rbtree_destroy(heap->tree, heap->all_nodes);
}

static struct { size_t len; csp_mem_heap_t *heaps; } csp_mem;

bool csp_mem_init(void) {
  csp_mem.heaps = (csp_mem_heap_t *)malloc(
    sizeof(csp_mem_heap_t) * csp_sched_np
  );
  if (csp_mem.heaps == NULL) {
    return false;
  }

  for (int i = 0; i < csp_sched_np; i++) {
    uintptr_t start = (uintptr_t)(i + 1) << csp_mem_heap_size_exp;
    if (!csp_mem_heap_init(&csp_mem.heaps[i], start)) {
      csp_mem.len = i;
      return false;
    }
  }

  csp_mem.len = csp_sched_np;
  return true;
}

void *csp_mem_alloc(size_t pid, size_t size) {
  return csp_mem_heap_alloc(&csp_mem.heaps[pid], size);
}

void csp_mem_free(size_t pid, void *obj) {
  csp_mem_heap_t *heap = &csp_mem.heaps[pid];

  /*
   * The condition check `csp_this_core == NULL` matters here cause it may be
   * the monitor thread in which the TLS variable `csp_this_core` is NULL.
   *
   * The call stack is:
   *
   *  csp_monitor()
   *  csp_monitor_poll()
   *  csp_netpoll_poll()
   *  csp_timer_cancel()
   *  csp_proc_destroy()
   *  csp_mem_free()
   */
  if (csp_this_core == NULL || csp_this_core->pid != pid) {
    csp_msrbq_push(obj)(
      heap->mailboxes[csp_mem_meta_l1_by_addr(heap, obj)], (uintptr_t)obj
    );
  } else {
    csp_mem_heap_free(heap, obj);
  }
}

void csp_mem_destroy(void) {
  for (int i = 0; i < csp_mem.len; i++) {
    csp_mem_heap_destroy(&csp_mem.heaps[i]);
  }
  free(csp_mem.heaps);
}
