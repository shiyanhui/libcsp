---
title: Mutex
---

## Overview

`mutex` implements the mutual exclusion locks. Although libcsp provides the `mutex`
synchronization primitives, try to avoid using it cause it may hurt the performance.

{{< hint info >}}
`Golang`: Do not communicate by sharing memory; instead, share memory by communicating.
{{< /hint >}}

## Index

- [csp_mutex_t](#csp_mutex_t)
- [csp_mutex_init(mutex)](#csp_mutex_initmutex)
- [csp_mutex_try_lock(mutex)](#csp_mutex_try_lockmutex)
- [csp_mutex_lock(mutex)](#csp_mutex_lockmutex)
- [csp_mutex_unlock(mutex)](#csp_mutex_unlockmutex)

### **csp_mutex_t**
---

`csp_mutex_t` defines the type of mutual exclusion locks.

Example:

```shell
csp_mutex_t mutex;
```

### **csp_mutex_init(mutex)**
---

`csp_mutex_init` initializes the mutex.

Example:

```shell
csp_mutex_init(&mutex);
```

### **csp_mutex_try_lock(mutex)**
---

`csp_mutex_try_lock` tries to lock the mutex. It returns `true` if success, otherwise `false`.

Example:

```shell
if (csp_mutex_try_lock(&mutex)) {
  // Do something...
  csp_mutex_unlock(&mutex);
}
```

### **csp_mutex_lock(mutex)**
---

`csp_mutex_lock` locks the mutex. It will spin if the mutex has been locked.

Example:

```shell
csp_mutex_lock(&mutex);
```

### **csp_mutex_unlock(mutex)**
---

`csp_mutex_unlock` unlocks the mutex.

Example:

```shell
csp_mutex_unlock(&mutex);
```
