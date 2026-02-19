#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "util.h"
#include <bus/event_bus.h>
#include <core/connection.h>
#include <core/event_loop.h>
#include <protocol/mcu_protocol.h>

// Path to the UNIX socket used for interprocess communication
#define UNIX_SOCKET_PATH "/tmp/gateway.sock"

int create_unix_server() {
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);

  struct sockaddr_un addr = {0};
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, UNIX_SOCKET_PATH);

  unlink(UNIX_SOCKET_PATH);

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind failed"); // Print error message if binding fails
    close(fd);             // Close the socket file descriptor on failure
    exit(EXIT_FAILURE);    // Exit the program with failure status
  } // Bind the socket to the specified address and port
  listen(fd, 16);

  set_nonblocking(fd);
  return fd;
}

void handle_unix_read(connection_t *conn) {
  char buf[256];

  int n = read(conn->fd, buf, sizeof(buf) - 1);
  if (n <= 0) {
    event_unsubscribe_all(conn);
    connection_close(conn);
    return;
  }

  buf[n] = 0;

  if (strncmp(buf, "SUB ", 4) == 0) {
    char *topic = buf + 4;
    topic[strcspn(topic, "\r\n")] = 0;
    event_subscribe(topic, conn);
  }
}

void handle_unix_accept(connection_t *listener) {
  while (1) {
    int client_fd = accept(listener->fd, NULL, NULL);
    if (client_fd < 0) {
      if (errno == EAGAIN)
        break;
      return;
    }

    set_nonblocking(client_fd);

    connection_t *conn = connection_create(listener->loop, client_fd);
    conn->on_read = handle_unix_read;
    conn->on_write = handle_write;

    event_loop_add(conn->loop, conn->fd, conn->events, conn);
  }
}

void transport_unix_init(event_loop_t *loop) {
  connection_t *listener = calloc(1, sizeof(connection_t));
  listener->fd = create_unix_server();
  listener->loop = loop;
  listener->on_read = handle_unix_accept;

  listener->events = EPOLLIN;
  event_loop_add(listener->loop, listener->fd, listener->events, listener);
  event_bus_init();
}
