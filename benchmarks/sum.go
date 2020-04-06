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

package main

import (
	"fmt"
	"sync"
	"time"
)

const N = 10

func sum(low, high int64, result *int64) {
	if low == high {
		*result = low
		return
	}

	var left, right int64
	mid := low + ((high - low) >> 1)

	var wg sync.WaitGroup
	wg.Add(2)

	go func() {
		sum(low, mid, &left)
		wg.Done()
	}()

	go func() {
		sum(mid+1, high, &right)
		wg.Done()
	}()

	wg.Wait()

	*result = left + right
}

func main() {
	var result int64

	start := time.Now()
	for i := 0; i < N; i++ {
		sum(0, 10000000, &result)
	}
	duration := time.Since(start)

	fmt.Printf("The result is %d, ran %d rounds, %f seconds per round.\n",
		result, N, float64(duration)/float64(time.Second)/float64(N),
	)
}
