---
type: docs
---

## Introduction

`libcsp` is a high performance concurrency C library influenced by the
[CSP](https://en.wikipedia.org/wiki/Communicating_sequential_processes) model.

## Features

- Multiple CPU cores supported.
- High performance scheduler.
- Stack size statically analyzed in compile time.
- Lock-free channel.
- Netpoll and timer are supported.

{{< columns >}}

### Go

```shell
go foo(arg1, arg2, arg3)

var wg sync.WaitGroup
wg.Add(2)
go func() { defer wg.Done(); foo(); }()
go func() { defer wg.Done(); bar(); }()
wg.Wait()

runtime.Gosched()

chn := make(chan int, 1 << 6)
num = <-chn
chn <- num

timer := time.AfterFunc(time.Second, foo)
timer.Stop()
```

<--->

### Libcsp

```shell
async(foo(arg1, arg2, arg3));

sync(foo(); bar());





yield();

chan_t(int) *chn = chan_new(int)(6);
chan_pop(chn, &num);
chan_push(chn, num);

timer_t timer = timer_after(timer_second, foo());
timer_cancel(timer);
```

{{< /columns >}}

