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

#define csp_without_prefix

#include <stdio.h>
#include <libcsp/csp.h>

proc void add(int a, int b, int c, int d, int e, int f, int g, int h) {
  printf("a: %d, b: %d, c: %d, d: %d, e: %d, f: %d, g: %d, h: %d\n",
    a, b, c, d, e, f, g, h
  );
}

int main(void) {
  sync(
    add(1, 2, 3, 4, 5, 6, 7, 8);
    add(9, 10, 11, 12, 13, 14, 15, 16);
  );
  return 0;
}
