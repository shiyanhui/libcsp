---
title: Timer
---

## Overview

The `timer` module provides the timer mechanism.

## Index

- [csp_timer_time_t](#csp_timer_time_t)
- [csp_timer_now()](#csp_timer_now)
- [csp_timer_duration_t](#csp_timer_duration_t)
- [csp_timer_t](#csp_timer_t)
- [csp_timer_at(when, task)](#csp_timer_atwhen-task)
- [csp_timer_after(duration, task)](#csp_timer_afterduration-task)
- [bool csp_timer_cancel(csp_timer_t timer)](#bool-csp_timer_cancelcsp_timer_t-timer)

### **csp_timer_time_t**
---

`csp_timer_time_t` is the type of timestamp in nanoseconds.

Example:

```shell
csp_timer_time_t now;
```

### **csp_timer_now()**
---

`csp_timer_now()` returns current timestamp.

Example:

```shell
csp_timer_time_t now = csp_timer_now();
```

### **csp_timer_duration_t**
---

`csp_timer_duration_t` is the type of duration between two timestamps in nanoseconds.
Libcsp provides several macros to describe the units.

- `csp_timer_nanosecond`
- `csp_timer_microsecond`
- `csp_timer_millisecond`
- `csp_timer_second`
- `csp_timer_minute`
- `csp_timer_hour`

Example:

```shell
csp_timer_time_t start = csp_timer_now();
csp_timer_duration_t duration = csp_timer_now() - start;
printf("Time slipped %ld nanoseconds\n", duration);
```

### **csp_timer_t**
---

`csp_timer_t` is the type of a timer. It is created by `csp_timer_at` or `csp_timer_after`.

### **csp_timer_at(when, task)**
---

`csp_timer_at(when, task)` creates and returns a timer which will fire at `when`.

- `when`: The timestamp in nanosecond when the timer fires.
- `task`: The task triggered when the timer fires.

Example:

```shell
void echo(const char *content) {
  printf("%s\n", content);
}
csp_timer_t timer = csp_timer_at(csp_timer_now() + csp_timer_second, echo("Timer fires"));
```

{{< hint warning >}}
`NOTE`: The task should be a single function call.
{{< /hint >}}

### **csp_timer_after(duration, task)**
---

`csp_timer_after(duration, task)` creates and returns a timer which will fire
after `duration` nanoseconds.

- `duration`: The duration after which the timer will fire.
- `task`: The task triggered when the timer fires.

Example:

```shell
csp_timer_t timer = csp_timer_after(csp_timer_second, echo("Timer fires"));
```

{{< hint warning >}}
`NOTE`: The task should be a single function call.
{{< /hint >}}

### **bool csp_timer_cancel(csp_timer_t timer)**
---

`csp_timer_cancel` cancels the timer. It will be ignored if the timer has fired
or been canceled. If success, it will return `true`, otherwise `false`.

Example:

```shell
csp_timer_cancel(timer);
```
