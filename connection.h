// Defines the buffer size for reading and writing data using TCP sockets.
#include <stdint.h>
#include "event_loop.h"
#define BUF_SIZE 1024

typedef enum {
  CONN_STATE_OPEN, // 连接已打开，正常状态
  CONN_STATE_READ_EOF, // 读取端已关闭（对方已关闭写入），但写入端仍可用
  CONN_STATE_WRITE_SHUTDOWN, // 写入端已关闭（对方已关闭读取），但读取端仍可用
  CONN_STATE_CLOSED // 连接完全关闭，无法读取或写入
} conn_state_t;

typedef struct connection connection_t;

typedef void (*conn_cb)(connection_t *);

struct connection {
  int fd;
  uint32_t events; // 当前监听的事件，例如 EPOLLIN、EPOLLOUT 等
  event_loop_t *loop; // 指向事件循环的指针，便于在回调中修改监听事件
  conn_state_t state; // 连接状态

  char inbuf[BUF_SIZE];
  int in_len;

  char *outbuf;
  int out_len;
  int out_cap;

  int read_closed;  // 标志：读取端是否已关闭
  int write_closed; // 标志：写入端是否已关闭

  int high_watermark; // 可选：用于实现流控的高水位线
  int low_watermark;  // 可选：用于实现流控的低水位线

  connection_t
      *peer; // 可选：指向相关联的连接，例如 MCU 连接可以指向对应的 UNIX 连接

  connection_t *next; // 可选：链表指针，用于管理多个连接

  conn_cb on_read;
  conn_cb on_write;
};
