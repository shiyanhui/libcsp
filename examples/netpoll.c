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

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <libcsp/csp.h>

char buf[1024];

proc void handle_conn(int conn) {
  if (!netpoll_register(conn)) {
    perror("register conn error");
    close(conn);
    return;
  }

  while (true) {
    ssize_t nread = 0;
    while (true) {
      nread = read(conn, buf, sizeof(buf));
      if (nread == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        netpoll_wait_read(conn, 0);
        continue;
      }
      if (nread <= 0) {
        goto failed;
      }
      break;
    }

    buf[nread] = '\0';
    printf("%s", buf);

    ssize_t nwrite = 0;
    while (nwrite < nread) {
      ssize_t n = write(conn, buf, nread);
      if (nwrite == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        netpoll_wait_write(conn, 0);
        continue;
      }
      if (nwrite < 0) {
        goto failed;
      }
      nwrite += n;
    }
  }

failed:
  perror("error");
  netpoll_unregister(conn);
  close(conn);
}

/*
 * This is a simple server example. It will echo anything it receives to the
 * client. You can communicate with it using telnet:
 *
 *  $ telnet 127.0.0.1 8080
 */
int main(void) {
  int sockfd = socket(AF_INET, SOCK_STREAM|SOCK_NONBLOCK, IPPROTO_TCP);
  if (sockfd <= 0) {
    perror("socket error\n");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in addr = {
    .sin_family = AF_INET,
    .sin_addr = {.s_addr=inet_addr("127.0.0.1")},
    .sin_port = htons(8080),
    .sin_zero = {0}
  };

  if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    perror("bind error\n");
    exit(EXIT_FAILURE);
  };

  if (listen(sockfd, 1024) != 0) {
    perror("listen error\n");
    exit(EXIT_FAILURE);
  }

  if (!netpoll_register(sockfd)) {
    perror("register sockfd error\n");
    exit(EXIT_FAILURE);
  }

  socklen_t len;
  struct sockaddr saddr;

  while (true) {
    int conn = accept(sockfd, &saddr, &len);
    if (conn >= 0) {
      async(handle_conn(conn));
    } else if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
      if (netpoll_wait_read(sockfd, csp_timer_second * 3) ==
          csp_netpoll_timeout) {
        printf("Timeout, will try again.\n");
      }
    } else {
      break;
    }
  }

  netpoll_unregister(sockfd);
  close(sockfd);
  return 0;
}
