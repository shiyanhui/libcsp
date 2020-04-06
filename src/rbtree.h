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

#ifndef LIBCSP_RBTREE_H
#define LIBCSP_RBTREE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include "common.h"

typedef struct csp_rbtree_node_t {
  int key;
  void *value;
  bool is_red;
  struct csp_rbtree_node_t *left, *right, *father;
} csp_rbtree_node_t;

csp_rbtree_node_t *csp_rbtree_node_new(int key, csp_rbtree_node_t *sentry) {
  csp_rbtree_node_t *node = (csp_rbtree_node_t *)malloc(
    sizeof(csp_rbtree_node_t)
  );
  if (node == NULL) {
    exit(EXIT_FAILURE);
  }

  node->key = key;
  node->value = NULL;
  node->is_red = true;
  node->left = node->right = node->father = sentry;

  return node;
}

/*
 * Rotate left subtree the root of which is `node`.
 *
 *    B                D
 *   / \              / \
 *  A   D     ->     B   E
 *     / \          / \
 *    C   E        A   C
 */
csp_rbtree_node_t *csp_rbtree_node_rotate_left(csp_rbtree_node_t *node) {
  csp_rbtree_node_t *right = node->right, *father = node->father;
  node->right = right->left;
  right->left->father = node;
  right->left = node;
  node->father = right;
  right->father = father;
  return right;
}

/*
 * Rotate right subtree the root of which is `node`.
 *
 *      D            B
 *     / \          / \
 *    B   E   ->   A   D
 *   / \              / \
 *  A   C            C   E
 */
csp_rbtree_node_t *csp_rbtree_node_rotate_right(csp_rbtree_node_t *node) {
  csp_rbtree_node_t *left = node->left, *father = node->father;
  node->left = left->right;
  left->right->father = node;
  left->right = node;
  node->father = left;
  left->father = father;
  return left;
}

void csp_rbtree_node_destroy(csp_rbtree_node_t *node) {
  free(node);
}

typedef struct csp_rbtree_t {
  csp_rbtree_node_t *root, *sentry, *stack[128];
  size_t nnodes;
} csp_rbtree_t;

csp_rbtree_t *csp_rbtree_new(void) {
  csp_rbtree_t *tree = (csp_rbtree_t *)malloc(sizeof(csp_rbtree_t));
  if (tree == NULL) {
    return NULL;
  }

  csp_rbtree_node_t *sentry = csp_rbtree_node_new(INT_MIN, NULL);
  sentry->left = sentry->right = sentry;
  sentry->is_red = false;

  tree->root = tree->sentry = sentry;
  tree->nnodes = 0;

  return tree;
}

/* Find a node the key of which is equal to the `key`. */
csp_rbtree_node_t *csp_rbtree_find(csp_rbtree_t *tree, int key) {
  csp_rbtree_node_t *node = tree->root;
  while (node != tree->sentry) {
    if (csp_unlikely(key == node->key)) {
      return node;
    }
    node = key < node->key ? node->left : node->right;
  }
  return NULL;
}

/* Find a node the key of which is greater than or equal to the `key`. */
csp_rbtree_node_t *csp_rbtree_find_gte(csp_rbtree_t *tree, int key) {
  csp_rbtree_node_t *node = tree->root, *greater = NULL;
  while (node != tree->sentry) {
    if (csp_unlikely(key == node->key)) {
      return node;
    }
    if (key < node->key) {
      greater = node;
      node = node->left;
    } else {
      node = node->right;
    }
  }
  return greater;
}

/* Insert a key to the tree. It returns the inserted or existing node. */
csp_rbtree_node_t *csp_rbtree_insert(csp_rbtree_t *tree, int key) {
  csp_rbtree_node_t **node = &tree->root, *father = tree->sentry, *curr,
    *grand, *uncle, *new_node;

  while (*node != tree->sentry) {
    if (csp_unlikely(key == (*node)->key)) {
      return *node;
    }
    father = *node;
    node = key < (*node)->key ? &(*node)->left : &(*node)->right;
  }

  curr = *node = new_node = csp_rbtree_node_new(key, tree->sentry);
  curr->father = father;
  tree->nnodes++;

  while (father != tree->sentry) {
    /* If current node is a 3-node, we just return. */
    if (!father->is_red) {
      return new_node;
    }

    /* The grand must be black since the father is red. */
    grand = father->father;

    /* If current node is a 5-node, we need to split it by flipping the color of
     * it's father, uncle and grand. */
    uncle = grand->left == father ? grand->right : grand->left;
    if (uncle->is_red) {
      father->is_red = uncle->is_red = false;
      grand->is_red = true;

      curr = grand;
      father = curr->father;
      continue;
    }

    /* Otherwise current node must be a 4-node. We need to check and fix it to
     * ensure it satisfies the red-black tree's propertity: "If a node is red,
     * then both its children are black.". */
    if (grand->left == father) {
      if (father->right == curr) {
        /*
         *     G          G
         *    /          /
         *   F    ->    C
         *    \        /
         *     C      F
         */
        grand->left = csp_rbtree_node_rotate_left(father);
      }
      /*
       *       G
       *      /         C
       *     C    ->   / \
       *    /         F   G
       *   F
       */
      curr = csp_rbtree_node_rotate_right(grand);
    } else {
      if (father->left == curr) {
        /*
         *   G        G
         *    \        \
         *     F  ->    C
         *    /          \
         *   C            F
         */
        grand->right = csp_rbtree_node_rotate_right(father);
      }
      /*
       *   G
       *    \           C
       *     C    ->   / \
       *      \       G   F
       *       F
       */
      curr = csp_rbtree_node_rotate_left(grand);
    }

    /* Flip the color of current node and grand. */
    grand->is_red = true;
    curr->is_red = false;

    /* Link original grand's father to current node. */
    father = curr->father;
    if (father == tree->sentry) {
      tree->root = curr;
    } else {
      *(father->left == grand ? &father->left : &father->right) = curr;
    }
    return new_node;
  }

  /* Always make the root of tree black. */
  tree->root->is_red = false;

  return new_node;
}

/* Delete `node` from the tree. It returns the successor of the deleted node if
 * the key and value of the successor is moved to another node. */
csp_rbtree_node_t *csp_rbtree_delete(csp_rbtree_t *tree,
    csp_rbtree_node_t *node) {
  csp_rbtree_node_t *next, *father, *sibling, *ret = NULL;

  /* If current node has two children, find and copy the key and value of its
   * successor to current node, and then delete its successor. So that we always
   * delete a leaf node or a inner node with only one child. */
  if (node->left != tree->sentry && node->right != tree->sentry) {
    csp_rbtree_node_t *succ = node->right;
    while (succ->left != tree->sentry) {
      succ = succ->left;
    }
    node->key = succ->key;
    node->value = succ->value;
    ret = node;
    node = succ;
  }

  /* Link `next` to the father of the deleted node. */
  father = node->father;
  next = node->left != tree->sentry ? node->left : node->right;
  next->father = father;

  /* The `next` may be the sentry or the only one child of the deleted node. We
   * can always make it black since if it's red then it must be a 3-node, and we
   * can make it a 2-node by setting it black. */
  bool is_3_or_4_node = node->is_red || next->is_red;
  next->is_red = false;

  csp_rbtree_node_destroy(node);
  tree->nnodes--;

  if (father == tree->sentry) {
    tree->root = next;
    return ret;
  }
  *(father->left == node ? &father->left : &father->right) = next;

  /* If it's a 3-node or 4-node, all restrictions of red-black tree are already
   * satisfied here, so we just return. */
  if (is_3_or_4_node) {
    return ret;
  }

  /* Otherwise the deleted node must be a 2-node, and rebalancing is needed. */
  while (father != tree->sentry) {
    if (father->left == next) {
      /* The positon of the sibling relies on the color of `father->right`. */
      if (!father->right->is_red) {
        sibling = father->right;
        /* If its sibling is a 2-node, then we merge it with its father. If its
         * father is a 3-node or 4-node, after merging it will be a 2-node or
         * 3-node which means all restrictions are satisfied and we can just
         * return. But if its father is a 2-node, underflow will happen after
         * merging, so we continue to rebalance the tree from its father. */
        if (!sibling->left->is_red && !sibling->right->is_red) {
          sibling->is_red = true;
          if (father->is_red) {
            father->is_red = false;
            return ret;
          }
          next = father;
          father = next->father;
          continue;
        }

        /* Otherwise we need to borrow a node from its sibling by rotating and
         * then all restrictions will be satisfied. */
        if (sibling->left->is_red) {
          father->right = csp_rbtree_node_rotate_right(father->right);
        } else {
          sibling->right->is_red = false;
        }
      } else {
        /* The sibling must be black since its father(i.e. father->right) is
         * red and it also can't be the sentry since it's in the same level with
         * the deleted node. */
        sibling = father->right->left;

        if (!sibling->left->is_red && !sibling->right->is_red) {
          /* If the sibling is a 2-node, merge it with its father. Since its
          * father is a 3-node, so all restrictions will be satisfied after
          * merging. */
          sibling->is_red= true;
        } else {
          /* Otherwise the sibling must be a 3-node or 4-node and we borrow a
           * node from it. */
          if (sibling->left->is_red) {
            father->right->left = csp_rbtree_node_rotate_right(sibling);
          } else {
            sibling->right->is_red = false;
          }
          father->right = csp_rbtree_node_rotate_right(father->right);
        }
      }
      next = csp_rbtree_node_rotate_left(father);
    } else {
      /* Handle the symmetrical case. */
      if (!father->left->is_red) {
        sibling = father->left;
        if (!sibling->right->is_red && !sibling->left->is_red) {
          sibling->is_red = true;
          if (father->is_red) {
            father->is_red = false;
            return ret;
          }
          next = father;
          father = next->father;
          continue;
        }
        if (sibling->right->is_red) {
          father->left = csp_rbtree_node_rotate_left(father->left);
        } else {
          sibling->left->is_red = false;
        }
      } else {
        sibling = father->left->right;
        if (!sibling->right->is_red && !sibling->left->is_red) {
          sibling->is_red= true;
        } else {
          if (sibling->right->is_red) {
            father->left->right = csp_rbtree_node_rotate_left(sibling);
          } else {
            sibling->left->is_red = false;
          }
          father->left = csp_rbtree_node_rotate_left(father->left);
        }
      }
      next = csp_rbtree_node_rotate_right(father);
    }

    /* Fix the color. */
    next->is_red = father->is_red;
    father->is_red = false;

    /* Link the father to the next. */
    if (next->father == tree->sentry) {
      tree->root = next;
    } else {
      *(next->father->left == father ? &next->father->left :
        &next->father->right) = next;
    }
    return ret;
  }
}

/* Get all nodes in the tree in in-order and return the number of nodes. */
size_t csp_rbtree_all_nodes(csp_rbtree_t *tree, csp_rbtree_node_t **nodes) {
  if (csp_unlikely(tree->nnodes == 0)) {
    return 0;
  }

  int nnodes = 0, nstack = 0;
  csp_rbtree_node_t *root = tree->root;

  while (root != tree->sentry || nstack > 0) {
    if (root != tree->sentry) {
      tree->stack[nstack++] = root;
      root = root->left;
    } else {
      root = tree->stack[--nstack];
      nodes[nnodes++] = root;
      root = root->right;
    }
  }
  return nnodes;
}

/* Destroy the tree and all nodes in it. */
void csp_rbtree_destroy(csp_rbtree_t *tree, csp_rbtree_node_t **nodes) {
  if (tree == NULL) {
    return;
  }

  if (tree->nnodes > 0) {
    size_t n = csp_rbtree_all_nodes(tree, nodes);
    for (size_t i = 0; i < n; i++) {
      csp_rbtree_node_destroy(nodes[i]);
    }
  }

  free(tree);
}

#ifdef __cplusplus
}
#endif

#endif
