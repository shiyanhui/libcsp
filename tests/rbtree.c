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

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include "../src/rbq.h"
#include "../src/rbtree.h"

#define N 10000

#define print_node(node, sentry)                                               \
  printf(                                                                      \
    "<Node %p key: %d, is_red: %d, left: %p, right: %p, father: %p> ",         \
    (node) == (sentry) ? NULL : (node),                                        \
    (node)->key,                                                               \
    (node)->is_red,                                                            \
    (node)->left == (sentry) ? NULL : (node)->left,                            \
    (node)->right == (sentry) ? NULL : (node)->right,                          \
    (node)->father == (sentry) ? NULL : (node)->father                         \
  );                                                                           \

csp_rrbq_declare(csp_rbtree_node_t *, node);
csp_rrbq_define(csp_rbtree_node_t *, node);

csp_rbtree_node_t *all_nodes[N];

void print_tree(csp_rbtree_t *tree) {
  if (tree->root == NULL) {
    return;
  }

  csp_rbtree_node_t *node;
  csp_rrbq_t(node) *queue = csp_rrbq_new(node)(10);

  csp_rrbq_try_push(node)(queue, tree->root);
  print_node(tree->root, tree->sentry);
  printf("\n");

  while (csp_rrbq_len(node)(queue) > 0) {
    int len = csp_rrbq_len(node)(queue);
    for (int i = 0; i < len; i++) {
      assert(csp_rrbq_try_pop(node)(queue, &node));
      if (node->left != tree->sentry) {
        assert(csp_rrbq_try_push(node)(queue, node->left));
        print_node(node->left, tree->sentry);
      }
      if (node->right != tree->sentry) {
        assert(csp_rrbq_try_push(node)(queue, node->right));
        print_node(node->right, tree->sentry);
      }
    }
    printf("\n");
  }

  csp_rrbq_destroy(node)(queue);
}

int csp_rbtree_verify_node(csp_rbtree_node_t *node, csp_rbtree_t *tree) {
  if (node == tree->sentry) {
    return 1;
  }

  if (node->is_red) {
    assert(!node->left->is_red);
    assert(!node->right->is_red);
  }

  if (node->left != tree->sentry) {
    assert(node->left->father == node);
  }
  if (node->right != tree->sentry) {
    assert(node->right->father == node);
  }

  int left = csp_rbtree_verify_node(node->left, tree);
  int right = csp_rbtree_verify_node(node->right, tree);
  assert(left == right);

  return left + !node->is_red;
}

void csp_rbtree_verify(csp_rbtree_t *tree) {
  assert(tree->root->father == tree->sentry);
  csp_rbtree_verify_node(tree->root, tree);
}

void test_rbtree_in_order(void) {
  const int max_num = N;

  csp_rbtree_t *tree = csp_rbtree_new();
  for (int i = 0; i < max_num; i++) {
    csp_rbtree_insert(tree, i);
    csp_rbtree_verify(tree);

    csp_rbtree_node_t *node = csp_rbtree_find(tree, i);
    assert(node != NULL);
    assert(node->key == i);
    assert(tree->nnodes == i + 1);

    node = csp_rbtree_find_gte(tree, i);
    assert(node != NULL);
    assert(node->key == i);
    assert(tree->nnodes == i + 1);
  }

  for (int i = 0; i < max_num; i++) {
    csp_rbtree_node_t *node = csp_rbtree_find(tree, i);
    assert(node);
    csp_rbtree_delete(tree, node);
    csp_rbtree_verify(tree);

    assert(csp_rbtree_find(tree, i) == NULL);

    node = csp_rbtree_find_gte(tree, i);
    if (i < max_num - 1) {
      assert(node != NULL);
      assert(node->key == i + 1);
    } else {
      assert(node == NULL);
    }
    assert(tree->nnodes == max_num - i - 1);
  }

  csp_rbtree_destroy(tree, all_nodes);
}

void test_rbtree_in_random(void) {
  const int max_num = N;

  csp_rbtree_t *tree = csp_rbtree_new();
  for (int i = 0; i < max_num; i++) {
    int num = rand() % max_num;
    csp_rbtree_insert(tree, num);
    csp_rbtree_verify(tree);

    csp_rbtree_node_t *node = csp_rbtree_find(tree, num);
    assert(node != NULL);
    assert(node->key == num);
  }

  for (int i = 0; i < max_num; i++) {
    csp_rbtree_node_t *node = csp_rbtree_find(tree, i);
    if (node == NULL) {
      continue;
    }
    csp_rbtree_delete(tree, node);
    csp_rbtree_verify(tree);
    assert(csp_rbtree_find(tree, i) == NULL);
  }

  csp_rbtree_destroy(tree, all_nodes);
}

void test_rbtree_in_random1(void) {
  const int max_num = N;

  csp_rbtree_t *tree = csp_rbtree_new();
  for (int i = 0; i < max_num; i++) {
    int num = rand() % max_num;

    if (num & 0x01) {
      csp_rbtree_insert(tree, num);
      csp_rbtree_verify(tree);
      csp_rbtree_node_t *node = csp_rbtree_find(tree, num);
      assert(node != NULL);
      assert(node->key == num);
    } else {
      csp_rbtree_node_t *node = csp_rbtree_find(tree, i);
      if (node == NULL) {
        continue;
      }
      csp_rbtree_delete(tree, node);
      csp_rbtree_verify(tree);
      assert(csp_rbtree_find(tree, i) == NULL);
    }
  }

  csp_rbtree_destroy(tree, all_nodes);
}

int main(void) {
  srand(time(NULL));
  test_rbtree_in_order();
  test_rbtree_in_random();
  test_rbtree_in_random1();
}
