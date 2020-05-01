---
title: Schedule
---

## Overview

The `schedule` module provides the functionality for controlling the behavior of
process.

## Index

- [csp_async(tasks)](#csp_asynctasks)
- [csp_sync(tasks)](#csp_synctasks)
- [csp_block(tasks)](#csp_blocktasks)
- [csp_yield()](#csp_yield)
- [csp_hangup(nanosec)](#csp_hangupnanosec)

### **csp_async(tasks)**
---

`csp_async(tasks)` runs tasks concurrently. If there are multiple tasks, you
need to separate them with `;` and libcsp will create and start a process for
each task. It's something like the `go` in golang.

Example:

```shell
void say(const char *word) {
  printf("%s\n", word);
}
csp_async(say("Hello world"));
csp_async(say("你好"); say("世界"));
```

{{< hint warning >}}
`NOTE`:
- All tasks should be non-pointer function call.
- The return of all function calls will be ignored.
{{< /hint >}}

### **csp_sync(tasks)**
---

`csp_sync(tasks)` works similarly to `csp_async(tasks)` except that it will block
until all tasks finish.

Example:

```shell
void add(int a, int b, int *sum) {
  *sum = a + b;
}

int x, y;
csp_sync(add(1, 2, &x); add(3, 4, &y));

printf("1 + 2 + 3 + 4 is %d\n", x + y);
```

{{< hint warning >}}
`NOTE`:
- All tasks should be non-pointer function call.
- The return of all function calls will be ignored.
{{< /hint >}}

### **csp_block(tasks)**
---

`csp_block(tasks)` is used to wrap tasks which will likely cause the thread to
block(e.g. syscall). libcsp will start another worker thread and keep current
thread running until all tasks finish.

Example:

```shell
int n;
char buff[256];

csp_block({
  n = read(fd, buff, sizeof(buff));
})
```

{{< hint warning >}}
`NOTE`:
- Task doesn't have to be function calls. It can be any c statement.
- Don't use any scheduling method(i.e. `csp_async`, `csp_sync`, `csp_block`, `csp_yield`,
  and `csp_hangup`) in tasks directly or indirectly.
{{< /hint >}}

### **csp_yield()**
---

`csp_yield()` gives up current CPU and schedules to run other processes. It's
something like `runtime.Gosched()` in go.

Example:

```shell
csp_yield();
```

### **csp_hangup(nanosec)**
---

`csp_hangup(nanosec)` make current process hang up for `nanosec` nanoseconds.

Example:

```shell
csp_hangup(100);
```
