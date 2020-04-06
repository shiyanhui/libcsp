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

#include <assert.h>
#include "../src/mem.c"

int csp_sched_np = 1;
_Thread_local csp_core_t *csp_this_core = &(csp_core_t){.pid = 0};
void csp_sched_yield(void) {}

void test_meta_index(void) {
  csp_mem_meta_index_t index = {0, 0, 0}, other = {1, 2, 3};
  assert(csp_mem_meta_index_is_zero(index));

  csp_mem_meta_index_set(index, other);
  assert(index[0] == 1);
  assert(index[1] == 2);
  assert(index[2] == 3);
  assert(!csp_mem_meta_index_is_zero(index));

  csp_mem_meta_index_set_by_l1l2(index, 0x12, 0x3456);
  assert(index[0] == 0x12);
  assert(index[1] == 0x34);
  assert(index[2] == 0x56);
  assert(!csp_mem_meta_index_is_zero(index));

  csp_mem_meta_index_set_zero(index);
  assert(index[0] == 0);
  assert(index[1] == 0);
  assert(index[2] == 0);
  assert(csp_mem_meta_index_is_zero(index));
}

void test_span(void) {
  csp_mem_span_t span;
  memset(&span, 0, sizeof(span));
  assert(csp_mem_span_npages_get(&span) == 0);

  csp_mem_span_npages_set(&span, 0x123456);
  assert(span.npages[0] == 0x12);
  assert(span.npages[1] == 0x34);
  assert(span.npages[2] == 0x56);
  assert(csp_mem_span_npages_get(&span) == 0x123456);
}

void test_page(void) {
  assert(csp_mem_page_size_exp == 12);
  assert(csp_mem_page_size == 4096);
}

void test_meta_l1(void) {
  assert(csp_mem_meta_l1_num_exp == 8);
  assert(csp_mem_meta_l1_num == 256);
  assert(csp_mem_meta_l1_size == 256 << 20);
  assert(csp_mem_meta_l1_size_mask == (256 << 20) - 1);
  assert((csp_mem_meta_l1_size & csp_mem_meta_l1_size_mask) == 0);

  csp_mem_meta_index_t index = {0x12, 0x34, 0x56};
  assert(csp_mem_meta_l1_by_index(index) == 0x12);

  csp_mem_heap_t heap = {.start = 0};
  assert(csp_mem_meta_l1_by_addr(&heap, 0x1112222000) == 0x111);
}

void test_meta_l2(void) {
  assert(csp_mem_meta_l2_num_exp == 16);
  assert(csp_mem_meta_l2_num == 65536);
  assert(csp_mem_meta_l2_num_mask == 65535);
  assert((csp_mem_meta_l2_num & csp_mem_meta_l2_num_mask) == 0);

  csp_mem_meta_index_t index = {0x12, 0x34, 0x56};
  assert(csp_mem_meta_l2_by_index(index) == 0x3456);

  csp_mem_heap_t heap = {.start = 0};
  assert(csp_mem_meta_l2_by_addr(&heap, 0x1112222000) == 8738);

  assert(csp_mem_meta_l1l2_to_addr(&heap, 0x111, 8738) == 0x1112222000);
}

void test_heap(void) {
  assert(csp_mem_heap_size_exp == 36);
  assert(csp_mem_heap_size == 64L << 30);

  csp_mem_heap_t heap = {.start = 0};
  assert(csp_mem_heap_offset(&heap, 0x111) == 0x111);
  assert(csp_mem_heap_init_l1(&heap, 0));
  assert(heap.metas[0] != NULL);
  assert(heap.mailboxes[0] != NULL);
  csp_mem_heap_destroy_l1(&heap, 0);
}

void test_arena(void) {
  assert(csp_mem_arena_size_exp == 24);
  assert(csp_mem_arena_size == 16 << 20);
  assert(csp_mem_arena_npages == 4096);

  csp_mem_heap_t heap;
  memset(&heap, 0, sizeof(heap));

  heap.start = 1L << csp_mem_heap_size_exp;
  heap.end = heap.start + csp_mem_heap_size;
  heap.curr = heap.start -= csp_mem_arena_size;

  void *arena = csp_mem_arena_new(&heap);
  int32_t l1 = csp_mem_meta_l1_by_addr(&heap, arena);
  assert(heap.metas[l1] != NULL);
  assert(heap.mailboxes[l1] != NULL);
  assert(heap.arenas != NULL);
  assert(heap.arenas->addr == arena);
  assert(heap.arenas->next == NULL);

  csp_mem_heap_destroy_l1(&heap, l1);
  munmap(heap.arenas->addr, csp_mem_arena_size);
  free(heap.arenas);
}

void test_meta(void) {
  csp_mem_meta_t *meta = csp_mem_meta_new(0);

  for (int i = 0; i < csp_mem_meta_l2_num; i++) {
    csp_mem_span_t *span = &meta->spans[i];
    assert(csp_mem_span_npages_get(span) == 0);
    assert(csp_mem_meta_l2_by_index(span->index) == i);
    assert(csp_mem_meta_index_is_zero(span->mt_pre));
    assert(csp_mem_meta_index_is_zero(span->mt_next));
    assert(csp_mem_meta_index_is_zero(span->fp_pre));
    assert(csp_mem_meta_index_is_zero(span->fp_next));
  }
  for (int i = 0; i < csp_mem_meta_l2_num / sizeof(uint8_t); i++) {
    assert(meta->taken_bits[i] == 0);
  }

  csp_mem_heap_t heap = {.start = 0};
  heap.metas[0] = meta;

  csp_mem_meta_index_t index = {0, 0, 7};
  int32_t l2 = csp_mem_meta_l2_by_index(index);
  assert(csp_mem_meta_taken_bit(&heap, 0, l2) == 0);
  assert(csp_mem_meta_taken_bit_by_index(&heap, index) == 0);

  csp_mem_meta_taken_bit_set(&heap, 0, 7);
  assert((meta->taken_bits[0] & 0x01) == 1);
  csp_mem_meta_taken_bit_clear(&heap, 0, 7);
  assert((meta->taken_bits[0] & 0x01) == 0);

  csp_mem_meta_taken_bit_set(&heap, 0, 8);
  assert(((meta->taken_bits[1] >> 7) & 0x01) == 1);
  csp_mem_meta_taken_bit_clear(&heap, 0, 8);
  assert(((meta->taken_bits[1] >> 7) & 0x01) == 0);

  for (int i = 0; i < csp_mem_meta_l2_num; i++) {
    csp_mem_meta_taken_bit_set(&heap, 0, i);
    assert(csp_mem_meta_taken_bit(&heap, 0, i));
    csp_mem_meta_taken_bit_clear(&heap, 0, i);
    assert(!csp_mem_meta_taken_bit(&heap, 0, i));
  }

  for (int i = 0; i < csp_mem_meta_l2_num; i++) {
    assert(csp_mem_meta_span_by_l1l2(&heap, 0, i) == &heap.metas[0]->spans[i]);
    csp_mem_meta_index_set_by_l1l2(index, 0, i);
    assert(csp_mem_meta_l2_by_index(index) == i);
    if (i == 0) {
      assert(csp_mem_meta_span_by_index(&heap, index) == NULL);
    } else {
      assert(csp_mem_meta_span_by_index(&heap, index) == &heap.metas[0]->spans[i]);
    }
  }

  assert(csp_mem_meta_span_by_addr(&heap, 0x1000) == &heap.metas[0]->spans[1]);
  csp_mem_meta_destroy(meta);
}

void test_tree_node(void) {
  assert(csp_mem_tree_node_num == 4096);

  csp_mem_heap_t heap = {
    .tree = csp_rbtree_new()
  };
  memset(&heap.cache_nodes, 0, sizeof(heap.cache_nodes));
  for (int i = 0; i < csp_mem_tree_node_num; i++) {
    assert(heap.cache_nodes[i] == NULL);
    assert(csp_mem_tree_node_cache_get(&heap, i) == NULL);
    assert(csp_mem_tree_node_get_gte(&heap, i) == NULL);
    assert(csp_mem_tree_node_get(&heap, i) == NULL);
  }

  csp_rbtree_node_t node;
  csp_mem_tree_node_cache_set(&heap, 0, &node);
  assert(csp_mem_tree_node_cache_get(&heap, 0) == &node);
  csp_mem_tree_node_cache_set(&heap, 0, NULL);
  assert(csp_mem_tree_node_cache_get(&heap, 0) == NULL);

  heap.metas[0] = csp_mem_meta_new(0);

  csp_mem_span_t *span, *pre, *next;
  csp_rbtree_node_t *node_1_page = csp_rbtree_insert(heap.tree, 1);

  /* Skip the first. */
  for (int i = 1; i < csp_mem_meta_l2_num; i++) {
    span = &(heap.metas[0]->spans[i]);
    assert(csp_mem_meta_l1_by_index(span->index) == 0);
    assert(csp_mem_meta_l2_by_index(span->index) == i);
    csp_mem_tree_node_put_span(&heap, node_1_page, span);
  }
  assert(csp_mem_tree_node_cache_get(&heap, 1) == node_1_page);

  span = (csp_mem_span_t *)node_1_page->value;
  for (int i = csp_mem_meta_l2_num - 1; i >= 1; i--) {
    assert(csp_mem_meta_l1_by_index(span->index) == 0);
    assert(csp_mem_meta_l2_by_index(span->index) == i);
    span = csp_mem_meta_span_by_index(&heap, span->fp_next);
  }

  /* Delete the second in the free span list. */
  span = csp_mem_meta_span_by_l1l2(&heap, 0, csp_mem_meta_l2_num - 2);
  csp_mem_tree_node_del_span(&heap, node_1_page, span);

  pre = csp_mem_meta_span_by_l1l2(&heap, 0, csp_mem_meta_l2_num - 1);
  next = csp_mem_meta_span_by_l1l2(&heap, 0, csp_mem_meta_l2_num - 3);

  assert(csp_mem_meta_index_is_zero(span->fp_pre));
  assert(csp_mem_meta_index_is_zero(span->fp_next));
  assert(csp_mem_meta_span_by_index(&heap, pre->fp_pre) == NULL);
  assert(csp_mem_meta_span_by_index(&heap, pre->fp_next) == next);
  assert(csp_mem_meta_span_by_index(&heap, next->fp_pre) == pre);

  /* Delete the first in the free span list. */
  span = csp_mem_meta_span_by_l1l2(&heap, 0, csp_mem_meta_l2_num - 1);
  csp_mem_tree_node_del_span(&heap, node_1_page, span);

  next = csp_mem_meta_span_by_l1l2(&heap, 0, csp_mem_meta_l2_num - 3);
  assert(node_1_page->value == next);
  assert(csp_mem_meta_index_is_zero(span->fp_pre));
  assert(csp_mem_meta_index_is_zero(span->fp_next));

  /* Delete the last in the free span list. */
  span = csp_mem_meta_span_by_l1l2(&heap, 0, 1);
  csp_mem_tree_node_del_span(&heap, node_1_page, span);

  pre = csp_mem_meta_span_by_l1l2(&heap, 0, 2);
  assert(csp_mem_meta_index_is_zero(span->fp_pre));
  assert(csp_mem_meta_index_is_zero(span->fp_next));
  assert(csp_mem_meta_span_by_index(&heap, pre->fp_next) == NULL);

  /* Delete all. */
  for (int i = csp_mem_meta_l2_num - 3; i >= 2; i--) {
    span = &(heap.metas[0]->spans[i]);
    csp_mem_tree_node_del_span(&heap, node_1_page, span);
    assert(csp_mem_meta_index_is_zero(span->fp_pre));
    assert(csp_mem_meta_index_is_zero(span->fp_next));
  }
  assert(csp_mem_tree_node_cache_get(&heap, 1) == NULL);

  csp_mem_meta_destroy(heap.metas[0]);
  csp_rbtree_destroy(heap.tree, heap.all_nodes);
}

int main(void) {
  test_page();
  test_span();
  test_meta_l1();
  test_meta_l2();
  test_meta_index();
  test_heap();
  test_arena();
  test_tree_node();
  test_meta();
}
