#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>
#include "state_machine.h"

void update_events(connection_t *conn) {
  event_loop_mod(conn->loop, conn->fd, conn->events, conn);
}

void disable_read(connection_t *conn) {
    if (conn->events & EPOLLIN) {
        conn->events &= ~EPOLLIN;
        update_events(conn);
    }
}

void disable_write(connection_t *conn) {
    if (conn->events & EPOLLOUT) {
        conn->events &= ~EPOLLOUT;
        update_events(conn);
    }
}

void enable_write(connection_t *conn) {
    if (!(conn->events & EPOLLOUT)) {
        conn->events |= EPOLLOUT;
        update_events(conn);
    }
}

void shutdown_write(connection_t *conn) {
    if (!conn->write_closed) {
        printf("shutdown write fd=%d\n", conn->fd);

        if (shutdown(conn->fd, SHUT_WR) < 0) {
            perror("shutdown");
        }

        conn->write_closed = 1;
    }
}

void close_connection(connection_t *conn) {
    printf("Closing connection fd=%d\n", conn->fd);
  event_loop_del(conn->loop, conn->fd);
    close(conn->fd);
    conn->fd = -1;
}

void connection_on_read_eof(connection_t *conn)
{
    conn->state = CONN_STATE_READ_EOF;
    disable_read(conn);

    if (conn->peer->out_len == 0) {
        shutdown_write(conn->peer);
    }
}

void connection_maybe_close(connection_t *conn)
{
    if (conn->state == CONN_STATE_WRITE_SHUTDOWN &&
        conn->peer->state == CONN_STATE_WRITE_SHUTDOWN)
    {
        close_connection(conn);
        close_connection(conn->peer);
    }
}
