// Defines the buffer size for reading and writing data using TCP sockets.
#define BUF_SIZE 1024

typedef enum { CONN_TCP_LISTENER, CONN_UNIX, CONN_MCU } conn_type_t;

typedef struct connection connection_t;

typedef void (*conn_cb)(connection_t *);

struct connection {
  int fd;
  conn_type_t type;

  char inbuf[BUF_SIZE];
  int in_len;

  char *outbuf;
  int out_len;
  int out_cap;

  int high_watermark; // 可选：用于实现流控的高水位线
  int low_watermark;  // 可选：用于实现流控的低水位线

  connection_t
      *peer; // 可选：指向相关联的连接，例如 MCU 连接可以指向对应的 UNIX 连接

  connection_t *next; // 可选：链表指针，用于管理多个连接

  conn_cb on_read;
  conn_cb on_write;
};
