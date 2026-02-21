#include <bus/event_bus.h>
#include <errno.h>
#include <protocol/mcu_protocol.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void handle_mcu_read(connection_t *conn) {
  while (1) {

    int n = read(conn->fd, conn->inbuf, sizeof(conn->inbuf));

    if (n > 0) {
      event_publish("sensor", conn->inbuf, n);

    } else if (n == 0) {
      printf("Connection fd=%d closed by peer\n", conn->fd);
      conn->state = CONN_STATE_READ_EOF;
      connection_close(conn);
      return;
    } else {
      if (errno != EAGAIN || errno != EWOULDBLOCK) {
        connection_close(conn);
        return;
      }
    }
  }
}

/**
 * Handles writing buffered data to the UNIX socket.
 * Checks if data exists in the UNIX socket buffer, writes as much as the socket
 * allows, and removes written data. Reacts to system errors (EAGAIN,
 * buffer-related issues) and modifies socket events dynamically.
 */
/**
 * Writes buffered data to a UNIX socket.
 *
 * This function is triggered when the socket is ready for writing.
 * It writes as much data as possible from the internal buffer to the
 * UNIX socket. If the buffer becomes empty, epoll write monitoring is
 * disabled to optimize performance.
 *
 * @param conn - Connection object associated with the UNIX socket.
 */
void handle_write(connection_t *conn) {
  if (conn->state == CONN_STATE_CLOSING) {
    event_unsubscribe_all(conn);
    connection_close(conn);
    return;
  }
  while (conn->out_len > 0) {

    int n = write(conn->fd, conn->outbuf, conn->out_len);

    if (n > 0) {

      memmove(conn->outbuf, conn->outbuf + n, conn->out_len - n);

      conn->out_len -= n;

    } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      printf("Unix socket not ready for writing, will retry later\n%s\n",
             strerror(errno));
      return;
    } else {
      event_unsubscribe_all(conn);
      connection_close(conn);
      return;
    }
  }

  connection_disable_write(conn);
}
