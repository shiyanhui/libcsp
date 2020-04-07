---
title: Netpoll
---

## Overview

The `netpoll` module provides the mechanism to interact with network poller(currently
only `epoll` supported).

## Example

```c
netpoll_register(sockfd);
while (true) {
  int conn = accept(sockfd, &addr, &len);
  if (conn >= 0) {
    async(handle_conn(conn));
  } else if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
    netpoll_wait_read(sockfd, 0);
  } else {
    break;
  }
}
csp_netpoll_unregister(sockfd);
```

Go to [netpoll.c](https://github.com/shiyanhui/libcsp/blob/master/examples/netpoll.c)
for the full example.

## Index

- [bool csp_netpoll_register(int fd)](#bool-csp_netpoll_registerint-fd)
- [int csp_netpoll_wait_read(int fd, csp_timer_duration_t timeout)](#int-csp_netpoll_wait_readint-fd-csp_timer_duration_t-timeout)
- [int csp_netpoll_wait_write(int fd, csp_timer_duration_t timeout)](#int-csp_netpoll_wait_writeint-fd-csp_timer_duration_t-timeout)
- [bool csp_netpoll_unregister(int fd)](#csp_netpoll_unregisterint-fd)

### **bool csp_netpoll_register(int fd)**
---

`csp_netpoll_register` registers `fd` to the netpoll so that the netpoll can
monitor io events related to the `fd`.

- `fd`: The file descriptor to register.

It returns `true` if success, otherwise `false`.

### **int csp_netpoll_wait_read(int fd, csp_timer_duration_t timeout)**
---

`csp_netpoll_wait_read` blocks until we can read from the `fd`.

- `fd`: The file descriptor registered.
- `timeout`: The duration we wait in nanoseconds. If it's `0` or negative, this
  argument will be ignored and it will block until we get related io events.

It returns,

- `csp_netpoll_avail` when available.
- `csp_netpoll_timeout` when timeout.

### **int csp_netpoll_wait_write(int fd, csp_timer_duration_t timeout)**
---

`csp_netpoll_wait_write` blocks until we can write to the `fd`.

- `fd`: The file descriptor registered.
- `timeout`: The duration we wait in nanoseconds. If it's `0` or negative, this
  argument will be ignored and it will block until we get related io events.

It returns,

- `csp_netpoll_avail` when available.
- `csp_netpoll_timeout` when timeout.

### **csp_netpoll_unregister(int fd)**
---

`csp_netpoll_unregister` unregisters `fd` from the netpoll.

- `fd`: The file descriptor to unregister.

It returns `true` if success, otherwise `false`.
