---
weight: 10
title: Tutorial
---

## Overview

In this tutorial we will write a stupid example in which we calculate the sum of two
numbers and print it.

## Basic version

Let's write a basic version first. It may look like,

```shell
#include <stdio.h>

int add(int a, int b) {
  return a + b;
}

int main(void) {
  printf("The sum of 1 and 2 is %d\n", add(1, 2));
  return 0;
}
```

Ok, now we want to calculate the sum in another process and print it in current
process. How can we achieve it?

## Async version

One solution is we do the calculating in another process and then get the result
through the channel.

{{< highlight shell "linenos=table" >}}
#define csp_without_prefix

#include <stdio.h>
#include <libcsp/csp.h>

chan_declare(ss, int, integer);
chan_define(ss, int, integer);

proc void add(int a, int b, chan_t(integer) *chn) {
  chan_push(chn, a + b);
}

int main(void){
  chan_t(integer) *chn = chan_new(integer)(0);
  async(add(1, 2, chn));

  int sum;
  chan_pop(chn, &sum);
  printf("The sum of 1 and 2 is %d\n", sum);

  chan_destroy(chn);
  return 0;
}
{{< / highlight >}}

There are many new things. Don't worry, let's review it line by line.

In the first line, we defined the macro `csp_without_prefix`. With it we can use
the libcsp's APIs without the prefix `csp_`.

In lines `6~7`, we declared and defined the a single-writer single-reader(that's
what `ss` means) channel consisting of `int` elements. We marked this channel with
label `integer` to avoid naming conflicts with other declared channel(e.g. channel
declared with `chan_declare(ss, int, num)`).

In lines `9~11`, we defined the calculating function. It has a parameter `chn`, thus
we can get the sum through the channel. The keyword `proc` is used to tell the
compiler not to inline the function. Libcsp relies on the `noinline`-ed task.

In lines `14`, we created a new channel object. And in line `15` we created and
started a new process to run the task `add(1, 2, chn)` by the libcsp API `async`.

In lines `17~19`, the process blocked until it got the result from the channel. Then
we printed the result.

In lines `21~22`, we released the channel resource and finally finished.

## Sync version

Ok, the async version solution works fine, congratulations. But wait, can we
improve it? Let's review the async version code again, and we found that,
- We create a new channel object, but we only use it once and then destroy it.
  The overhead is somewhat heavy.
- The main process relies on the task process. It will block until the task process
  finishes and thus it shouldn't be tried to be scheduled by the libcsp scheduler
  meanwhile .

`libcsp` provides mechanism to solve this, it's `sync`.  Below is the sync version
solution.

```shell
#define csp_without_prefix

#include <stdio.h>
#include <libcsp/csp.h>

proc void add(int a, int b, int *sum) {
  *sum = a + b;
}

int main(void){
  int sum;
  sync(add(1, 2, &sum));
  printf("The sum of 1 and 2 is %d\n", sum);
  return 0;
}
```

Compared to the async version, it's shorter and faster.

## Build

Assume the file is `sum.c`, we build it with following commands,

```shell
cspcli init
gcc -o sum.o -c sum.c -fplugin=libcsp
cspcli analyze
gcc -o sum sum.o /tmp/libcsp/config.c -lcsp -pthread
```

Go to [Building](/building) for more detail about the building.
