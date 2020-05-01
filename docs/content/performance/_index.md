---
weight: 50
title: Performance
---

## Overview

It's hard to do a full benchmark for the scheduler. We need to choose a scenario
in which the scheduler will be busy creating, scheduling, running and destroying
the processes.

We continue to talk about the topic of the `sum` example in the tutorial. But
now the computing rule is changed to a divide-and-conquer one: `sum(low, high)
= sum(low, mid) + sum(mid + 1, high)` in which `mid` is `(low + high) / 2`.

In a basic version, it may look like:

```c
int sum(int low, int high) {
  if (low == high) {
    return low;
  }
  int mid = low + ((high - low) >> 1);
  return sum(low, mid) + sum(mid + 1, high);
}
```

In our benchmark, we compute the sub-parts `sum` in two processes each. And it
becomes:

```c
proc void sum(int low, int high, int *result) {
  if (low == high) {
    *result = low;
    return;
  }
  int mid = low + ((high - low) >> 1), left, right;
  sync(sum(low, mid, &left); sum(mid + 1, high, &right));
  *result = left + right;
}
```

Go to [sum.go](https://github.com/shiyanhui/libcsp/tree/master/benchmarks/sum.go) for
the equivalent golang function.

## Environment

- OS: Linux ubuntu-bionic 4.15.0-88-generic x86_64
- CPU Cores: 8
- Memory: 8192M
- Go: v1.14
- GCC: v8.3.0

## Benchmark

Now compute `sum(0, 10000000)` for `10` rounds and print the spent time per
round in seconds.

```shell
$ for i in $(seq 8); do make clean && make CC=gcc-8 CPU_CORES=${i} benchmark_sum; done
The result is 50000005000000, ran 10 rounds, 3.047471 seconds per round.
The result is 50000005000000, ran 10 rounds, 1.579780 seconds per round.
The result is 50000005000000, ran 10 rounds, 1.082895 seconds per round.
The result is 50000005000000, ran 10 rounds, 0.847325 seconds per round.
The result is 50000005000000, ran 10 rounds, 0.787095 seconds per round.
The result is 50000005000000, ran 10 rounds, 0.744351 seconds per round.
The result is 50000005000000, ran 10 rounds, 0.717903 seconds per round.
The result is 50000005000000, ran 10 rounds, 0.726390 seconds per round.

$ for i in $(seq 8); do make clean && env GOMAXPROCS=${i} make benchmark_sum_go; done
The result is 50000005000000, ran 10 rounds, 31.145248 seconds per round.
The result is 50000005000000, ran 10 rounds, 17.112188 seconds per round.
The result is 50000005000000, ran 10 rounds, 12.460803 seconds per round.
The result is 50000005000000, ran 10 rounds, 11.172224 seconds per round.
The result is 50000005000000, ran 10 rounds, 9.097314 seconds per round.
The result is 50000005000000, ran 10 rounds, 8.355515 seconds per round.
The result is 50000005000000, ran 10 rounds, 8.135163 seconds per round.
The result is 50000005000000, ran 10 rounds, 8.029795 seconds per round.
```

Below is the result in table:

| CPU Cores | Time(s) libcsp/go    | Memory(%) libcsp/go |
| :-------- | :------------------- | ------------------- |
| 1         | 3.047471 / 31.145248 | 0.2 / 27.8          |
| 2         | 1.579780 / 17.112188 | 0.5 / 29.4          |
| 3         | 1.082895 / 12.460803 | 1.0 / 28.9          |
| 4         | 0.847325 / 11.172224 | 1.2 / 30.7          |
| 5         | 0.787095 / 9.097314  | 1.6 / 32.0          |
| 6         | 0.744351 / 8.355515  | 1.9 / 29.5          |
| 7         | 0.717903 / 8.135163  | 2.0 / 29.5          |
| 8         | 0.726390 / 8.029795  | 2.3 / 31.2          |


We will see that libcsp is at least `10` times faster and use less memory than
golang in the benchmark.
