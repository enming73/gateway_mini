#include "mcu_protocol.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void handle_read(connection_t *conn) {
  while (1) {

    int n = read(conn->fd, conn->inbuf, sizeof(conn->inbuf));

    if (n > 0) {

      // unix_append(conn->peer, conn->inbuf, n);
      connection_append_out(conn->peer, conn->inbuf, n);

    } else if (n == 0) {
      printf("Connection fd=%d closed by peer\n", conn->fd);
      conn->state = CONN_STATE_READ_EOF;
      connection_disable_read(conn);
      if (conn->peer && conn->peer->out_len == 0) {
        connection_shutdown_write(conn->peer);
      }
      return;
    } else {

      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return;

      connection_t *peer = conn->peer;

      connection_close(conn);
      if (peer) {
        connection_close(peer);
      }
      return;
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
  while (conn->out_len > 0) {

    int n = write(conn->fd, conn->outbuf, conn->out_len);

    if (n > 0) {

      memmove(conn->outbuf, conn->outbuf + n, conn->out_len - n);

      conn->out_len -= n;

    } else if (errno == EAGAIN) {
      printf("Unix socket not ready for writing, will retry later\n%s\n",
             strerror(errno));
      return;
    } else {
      connection_close(conn);
      return;
    }
  }

  /*if (conn->out_len == 0) {

    disable_write(conn);

    if (conn->state == CONN_STATE_READ_EOF) {
      shutdown_write(conn);
    }

    connection_maybe_close(conn);
  }*/
  connection_disable_write(conn);

  if (conn->out_len <= conn->low_watermark) {
    if (conn->peer) {
      printf("Unix buffer below low watermark, resuming reads from peer "
             "connection fd=%d\n",
             conn->peer->fd);
      connection_enable_read(conn->peer);
    }
  }
}
