#ifndef CONNECTION_H
#define CONNECTION_H

// Defines the buffer size for reading and writing data using TCP sockets.
#include <stdint.h>
#include <sys/epoll.h>
#define BUF_SIZE 1024

typedef struct event_loop event_loop_t;

typedef enum {
  CONN_STATE_OPEN, // 连接已打开，正常状态
  CONN_STATE_READ_EOF, // 读取端已关闭（对方已关闭写入），但写入端仍可用
  CONN_STATE_WRITE_SHUTDOWN, // 写入端已关闭（对方已关闭读取），但读取端仍可用
  CONN_STATE_CLOSED // 连接完全关闭，无法读取或写入
} conn_state_t;

typedef struct connection {
  int fd;
  uint32_t events; // 当前监听的事件，例如 EPOLLIN、EPOLLOUT 等
  event_loop_t *loop; // 指向事件循环的指针，便于在回调中修改监听事件
  conn_state_t state; // 连接状态

  char inbuf[4096];
  int in_len;

  char *outbuf;
  int out_len;
  int out_cap;

  int read_closed;  // 标志：读取端是否已关闭
  int write_closed; // 标志：写入端是否已关闭

  int high_watermark; // 可选：用于实现流控的高水位线
  int low_watermark;  // 可选：用于实现流控的低水位线

  struct connection
      *peer; // 可选：指向相关联的连接，例如 MCU 连接可以指向对应的 UNIX 连接

  struct connection *next; // 可选：链表指针，用于管理多个连接

  void (*on_read)(struct connection *); // 读取事件回调函数，参数为当前连接指针
  void (*on_write)(struct connection *); // 写入事件回调函数，参数为当前连接指针

  void *user_data; // 可选：指向用户数据的指针，便于在回调中存储上下文信息
} connection_t;

connection_t *connection_create(event_loop_t *loop, int fd);
void connection_destroy(connection_t *conn);

void connection_enable_read(connection_t *conn);
void connection_disable_read(connection_t *conn);
void connection_enable_write(connection_t *conn);
void connection_disable_write(connection_t *conn);

void connection_shutdown_write(connection_t *conn);
void connection_close(connection_t *conn);

void connection_append_out(connection_t *conn, const char *data, int len);

#endif // CONNECTION_H
