---
title: Channel
---

## Overview

Channel is implemented as a lock-free and thread-safe ring buffer inspired by
`LMAX Disruptor`. It is optimized specially for different combinations of
writers and readers,

- `ss`: `single` writer and `single` reader.
- `sm`: `single` writer and `multiple` readers.
- `ms`: `multiple` writers and `single` reader.
- `mm`: `multiple` writers and `multiple` readers.

## Index

- [csp_chan_declare(K, T, I)](#csp_chan_declarek-t-i)
- [csp_chan_define(K, T, I)](#csp_chan_definek-t-i)
- [csp_chan_t(I)](#csp_chan_ti)
- [csp_chan_new(I)](#csp_chan_newi)
- [csp_chan_try_push(chn, item)](#csp_chan_try_pushchn-item)
- [csp_chan_push(chn, item)](#csp_chan_pushchn-item)
- [csp_chan_try_pop(chn, item)](/#csp_chan_try_popchn-item)
- [csp_chan_pop(chn, item)](#csp_chan_popchn-item)
- [csp_chan_try_pushm(chn, items, n)](#csp_chan_try_pushmchn-items-n)
- [csp_chan_pushm(chn, items, n)](#csp_chan_pushmchn-items-n)
- [csp_chan_try_popm(chn, items, n)](#csp_chan_try_popmchn-items-n)
- [csp_chan_popm(chn, items, n)](#csp_chan_popmchn-items-n)
- [csp_chan_destroy(chn)](#csp_chan_destroyc)

### **csp_chan_declare(K, T, I)**
---

`csp_chan_declare(K, T, I)` declares the `channel` related functions prototypes.
It is usually used in the `.h` file.

- K: `Kind` of the channel, i.e. `ss`, `sm`, `ms` or `mm`.
- T: `Type` of elements in the channel, e.g. `int`.
- I: `Identifier` of the channel. It's used to avoid naming conflicts with other channels.

Example:

```shell
csp_chan_declare(ss, int, integer);
```

### **csp_chan_define(K, T, I)**
---

`csp_chan_define(K, T, I)` implements the declared channel. `K`, `T` and `I`
must be the same as that in `csp_chan_declare`. It is usually used in the `.c`
file.

Example:

```shell
csp_chan_define(ss, int, integer);
```

### **csp_chan_t(I)**
---

`csp_chan_t(I)` is the type of the declared channel. `I` is identifier of the channel.

Example:

```shell
csp_chan_t(integer) *chn;
```

### **csp_chan_new(I)**
---

`csp_chan_new(I)` creates a new channel object. It has one parameter,

- `size_t exp`: it means the exponent of the channel capacity, i.e. `capacity = 2^exp`.

It returns pointer to the channel if success, otherwise `NULL`.

Example:

```shell
// The capacity is 2^6, i.e. 64.
csp_chan_t(integer) *chn = csp_chan_new(integer)(6);
```

### **csp_chan_try_push(chn, item)**
---

`csp_chan_try_push(chn, item)` tries to push an item to the channel.

- `chn`: The channel.
- `item`: The item to push.

It will returns `true` if success, otherwise `false`.

Example:

```shell
if (csp_chan_try_push(chn, 1024)) {
  printf("pushed!\n");
}
```

### **csp_chan_push(chn, item)**
---

`csp_chan_push(chn, item)` pushes an item to the channel. It will block until it
successes.

- `chn`: The channel.
- `item`: The item to push.

Example:

```shell
csp_chan_push(chn, 1024);
```

### **csp_chan_try_pop(chn, item)**
---

`csp_chan_try_pop(chn, item)` tries to pop an item from the channel.

- `chn`: The channel.
- `item`: The place to put the popped item.

It will return `true` if success, otherwise `false`.

Example:

```shell
int num;
if (csp_chan_try_pop(chn, &num)) {
  printf("poped number is %d\n", num);
}
```

### **csp_chan_pop(chn, item)**
---

`csp_chan_pop(chn, item)` pops an item from the channel. It will block until it
successes.

- `chn`: The channel.
- `item`: The place to put the popped item.

Example:

```shell
int num;
csp_chan_pop(chn, &num);
```

### **csp_chan_try_pushm(chn, items, n)**
---

`csp_chan_try_pushm(chn, items, n)` tries to push `n` items to the channel.

- `chn`: The channel.
- `items`: The item to push.
- `n`: The number of items we want to push.

It will returns `true` if success, otherwise `false`.

Example:

```shell
int nums[] = {1, 2, 3, 4, 5, 6, 7, 8};
if (csp_chan_try_pushm(chn, nums, sizeof(nums)/sizeof(int))) {
  printf("pushed!\n");
}
```

### **csp_chan_pushm(chn, items, n)**
---

`csp_chan_pushm(chn, items, n)` push `n` items to the channel. It will block
until it successes.

- `chn`: The channel.
- `items`: The item to push.
- `n`: The number of items we want to push.

Example:

```shell
int nums[] = {1, 2, 3, 4, 5, 6, 7, 8};
csp_chan_pushm(chn, nums, sizeof(nums)/sizeof(int));
```

### **csp_chan_try_popm(chn, items, n)**
---

`csp_chan_try_popm(chn, items, n)` tries to pop multiple items from the channel.

- `chn`: The channel.
- `items`: The place to put the popped items.
- `n`: The capacity of the `items`.

It will return the number of items popped.

Example:

```shell
int nums[8];
size_t n = csp_chan_try_popm(chn, nums, sizeof(nums)/sizeof(int));
printf("poped %ld numbers\n", n);
```

### **csp_chan_popm(chn, items, n)**
---

`csp_chan_popm(chn, items, n)` pop `n` items from the channel. It will block until
it successes.

- `chn`: The channel.
- `items`: The place to put the popped items.
- `n`: The number of items we want to pop.

Example:

```shell
int nums[8];
csp_chan_popm(chn, nums, sizeof(nums)/sizeof(int));
```

### **csp_chan_destroy(chn)**
---

`csp_chan_destroy(chn)` destroys the channel.

Example:

```shell
csp_chan_destroy(chn);
```
