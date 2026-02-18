#include "connection.h"
#include "event_loop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

connection_t *connection_create(event_loop_t *loop, int fd) {
  connection_t *conn = calloc(1, sizeof(connection_t));
  conn->fd = fd;
  conn->loop = loop;
  conn->events = EPOLLIN; // 默认监听可读事件

  conn->out_cap = 4096;                 // 初始输出缓冲区容量
  conn->outbuf = malloc(conn->out_cap); // 输出缓冲区，动态分配

  conn->high_watermark = 8192; // 可选：设置高水位线，单位为字节
  conn->low_watermark = 4096;  // 可选：设置低水位线，单位为字节

  conn->state = CONN_STATE_OPEN; // 初始状态为打开

  return conn;
}

void connection_destroy(connection_t *conn) {
  if (!conn)
    return;
  free(conn->outbuf);
  free(conn);
}

void connection_enable_read(connection_t *conn) {
  if (!(conn->events & EPOLLIN)) {
    conn->events |= EPOLLIN;
    event_loop_mod(conn->loop, conn->fd, conn->events, conn);
  }
}
void connection_disable_read(connection_t *conn) {
  if (conn->events & EPOLLIN) {
    conn->events &= ~EPOLLIN;
    event_loop_mod(conn->loop, conn->fd, conn->events, conn);
  }
}

void connection_enable_write(connection_t *conn) {
  if (!(conn->events & EPOLLOUT)) {
    conn->events |= EPOLLOUT;
    event_loop_mod(conn->loop, conn->fd, conn->events, conn);
  }
}

void connection_disable_write(connection_t *conn) {
  if (conn->events & EPOLLOUT) {
    conn->events &= ~EPOLLOUT;
    event_loop_mod(conn->loop, conn->fd, conn->events, conn);
  }
}

void connection_shutdown_write(connection_t *conn) {
  if (!conn->write_closed) {
    printf("shutdown write fd=%d\n", conn->fd);

    if (shutdown(conn->fd, SHUT_WR) < 0) {
      perror("shutdown");
    }

    conn->write_closed = 1;
    conn->state = CONN_STATE_WRITE_SHUTDOWN;
  }
}

void connection_close(connection_t *conn) {
  printf("Closing connection fd=%d\n", conn->fd);
  event_loop_del(conn->loop, conn->fd);
  close(conn->fd);
  connection_destroy(conn);
}

void connection_append_out(connection_t *conn, const char *data, int len) {
  // 确保输出缓冲区有足够的空间
  if (conn->out_len + len > conn->out_cap) {
    int new_cap = conn->out_cap * 2;
    while (new_cap < conn->out_len + len) {
      new_cap *= 2;
    }
    char *new_buf = realloc(conn->outbuf, new_cap);
    if (!new_buf) {
      perror("realloc");
      return;
    }
    conn->outbuf = new_buf;
    conn->out_cap = new_cap;
  }
  int was_empty = (conn->out_len == 0);
  // 将数据追加到输出缓冲区
  memcpy(conn->outbuf + conn->out_len, data, len);
  conn->out_len += len;
  // 启用写事件以便发送数据
  if (was_empty) {
    connection_enable_write(conn);
  }

  if (conn->out_len >= conn->high_watermark) {
    printf("Unix buffer reached high watermark, pausing reads from peer\n");
    // 可选：实现流控逻辑，例如暂停读取更多数据
    if (conn->peer) {
      printf("Pausing reads from peer connection fd=%d\n", conn->peer->fd);
      connection_disable_read(conn->peer);
    }
  }
}
