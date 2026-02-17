// Enabling GNU extensions for additional socket and file control functions
#define _GNU_SOURCE
// Header for Internet operations (e.g., sockaddr_in, htons, etc.)
#include <arpa/inet.h>
// Defines error codes for detecting and handling error conditions
#include <errno.h>
// Provides file control operations (e.g., setting socket to non-blocking mode)
#include <fcntl.h>
// Standard input/output operations (e.g., printf for debugging)
#include <stdio.h>
// General utilities (e.g., memory allocation, exit, etc.)
#include <stdlib.h>
// String manipulation functions (e.g., memcpy, strcpy, memset)
#include <string.h>
// Epoll library for scalable I/O event notification
#include <sys/epoll.h>
// Socket programming functions (e.g., socket, bind, listen, connect)
#include <sys/socket.h>
// Definitions for UNIX domain sockets
#include <sys/un.h>
// POSIX API for system calls (e.g., close, read, write)
#include <unistd.h>

#include "state_machine.h" // Include the header file defining connection structures and constants

/** Maximum queued epoll events processed per invocation. */
#define MAX_EVENTS 64
// Port number for the TCP server to listen on
#define TCP_PORT 9000
// Path to the UNIX socket used for interprocess communication
#define UNIX_SOCKET_PATH "/tmp/gateway.sock"
// Maximum size of the buffer for outgoing UNIX socket data
#define UNIX_OUT_BUF 65536

/**
 * Sets the provided file descriptor to non-blocking mode.
 *
 * @param fd - The file descriptor to modify.
 * @return 0 on success, or -1 if an error occurs.
 */
static int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/**
 * Creates and sets up a non-blocking TCP server socket.
 * Binds the socket to the specified address and port, sets it to listen mode
 * with a specified backlog, and makes it non-blocking.
 * If any initialization step fails, the function prints an error message and
 * exits.
 */
/**
 * Creates and initializes a non-blocking TCP server socket.
 *
 * This function sets up a TCP server that listens on a specified port.
 * It binds the socket to all available interfaces and ensures it is ready
 * for accepting incoming connections.
 *
 * @return The file descriptor of the created TCP server socket.
 */
int create_tcp_server() {
  int fd = socket(AF_INET, SOCK_STREAM, 0); // Create a TCP socket

  int opt = 1; // Option to allow immediate reuse of the address
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt,
             sizeof(opt)); // Set socket options

  struct sockaddr_in addr = {0}; // Initialize the server's address structure
  addr.sin_family = AF_INET;     // Use IPv4 addressing
  addr.sin_addr.s_addr = INADDR_ANY; // Bind to all available interfaces
  addr.sin_port = htons(TCP_PORT);   // Convert and assign the port number

  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind failed"); // Print error message if binding fails
    close(fd);             // Close the socket file descriptor on failure
    exit(EXIT_FAILURE);    // Exit the program with failure status
  } // Bind the socket to the specified address and port
  listen(fd,
         128); // Start listening for incoming connections with a backlog of 128

  set_nonblocking(fd);
  return fd;
}

/**
 * Creates and connects to a UNIX domain socket.
 * This function initializes the socket, sets its address, and connects to the
 * specified UNIX domain server. If the process fails (e.g., address
 * unavailable, permission issues), it logs an error and terminates.
 */
/**
 * Creates and connects to a non-blocking UNIX domain socket.
 *
 * This function sets up a socket to allow communication over the UNIX-domain.
 * It attempts to connect to a server specified by a file path.
 *
 * @return The file descriptor of the UNIX domain socket upon success.
 */
int create_unix_client() {
  int fd = socket(AF_UNIX, SOCK_STREAM, 0); // Create a UNIX domain socket

  struct sockaddr_un addr = {0}; // Initialize the socket address structure
  addr.sun_family = AF_UNIX;     // Address family set to UNIX domain sockets
  strcpy(addr.sun_path,
         UNIX_SOCKET_PATH); // Copy the socket path to the address structure

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("connect to unix socket failed"); // Print error message if
                                             // connection fails
    close(fd);          // Close the socket file descriptor on failure
    exit(EXIT_FAILURE); // Exit the program with failure status
  } // Attempt to connect to the specified UNIX socket
  set_nonblocking(fd);
  return fd;
}


void pause_read(connection_t *conn) {
  disable_read(conn);
}

/**
 * Appends data to a UNIX domain socket connection buffer.
 *
 * This function places the provided `data` into the static UNIX socket
 * buffer. If the buffer was empty, the epoll event for the socket is
 * updated to include write readiness monitoring.
 *
 * @param unix_conn - The UNIX socket connection.
 * @param data      - The data to append to the write buffer.
 * @param len       - The length of the data to append.
 */
void unix_append(connection_t *conn, const char *data, int len) {
  if (conn->out_len + len > conn->out_cap) {

    int new_cap = conn->out_cap * 2;
    while (new_cap < conn->out_len + len)
      new_cap *= 2;

    char *new_buf = realloc(conn->outbuf, new_cap);
    if (!new_buf) {
      perror("realloc");
      return;
    }

    conn->outbuf = new_buf;
    conn->out_cap = new_cap;
  }

  int was_empty = (conn->out_len == 0);

  memcpy(conn->outbuf + conn->out_len, data, len);
  conn->out_len += len;

  if (was_empty) {
    enable_write(conn);
  }

  if (conn->out_len >= conn->high_watermark) {
    printf("Unix buffer reached high watermark, pausing reads from peer\n");
    // 可选：实现流控逻辑，例如暂停读取更多数据
    if (conn->peer) {
      printf("Pausing reads from peer connection fd=%d\n", conn->peer->fd);
      pause_read(conn->peer);
    }
  }
}

void close_pair(connection_t *conn) {
  connection_t *peer = conn->peer;

  event_loop_del(conn->loop, conn->fd);
  close(conn->fd);
  free(conn->outbuf);
  free(conn);

  if (peer) {
    event_loop_del(peer->loop, peer->fd);
    close(peer->fd);
    free(peer->outbuf);
    free(peer);
  }
}


void handle_read(connection_t *conn) {
  while (1) {

    int n = read(conn->fd, conn->inbuf, sizeof(conn->inbuf));

    if (n > 0) {

      unix_append(conn->peer, conn->inbuf, n);

    } else if (n == 0) {
      printf("Connection fd=%d closed by peer\n", conn->fd);
      connection_on_read_eof(conn);
      return;
    } else {

      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return;

      close_pair(conn);
      return;
    }
  }
}

void init_mcu_connection(connection_t *conn) {
  conn->out_cap = 4096;
  conn->outbuf = malloc(conn->out_cap);
  conn->high_watermark = 8 * 1024;
  conn->low_watermark = 4 * 1024;
}

void resume_read(connection_t *conn) {
  if (!(conn->events & EPOLLIN)) {
        conn->events |= EPOLLIN;
        update_events(conn);
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

    } else if (n == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      printf("Unix socket not ready for writing, will retry later\n%s\n",
             strerror(errno));
      return;
    } else {
      perror("unix write");
      return;
    }
  }

  if (conn->out_len == 0) {

    disable_write(conn);

    if (conn->state == CONN_STATE_READ_EOF) {
        shutdown_write(conn);
    }
    
    connection_maybe_close(conn);

  }

  if (conn->out_len <= conn->low_watermark) {
    if (conn->peer) {
      printf("Unix buffer below low watermark, resuming reads from peer "
             "connection fd=%d\n",
             conn->peer->fd);
      resume_read(conn->peer);
    }
  }
}
/**
 * Accepts new incoming connections to the listener.
 *
 * This function is triggered when the TCP listener has pending
 * connections. It accepts the connections, initializes them, and
 * adds them to the epoll instance for event management.
 *
 * @param listener - Listener object.
 */
void handle_accept(connection_t *listener) {
  printf("accept triggered\n");
  while (1) {
    int client_fd = accept(listener->fd, NULL, NULL);
    if (client_fd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;
      perror("accept");
      break;
    }
    set_nonblocking(client_fd);

    connection_t *tcp_conn = calloc(1, sizeof(connection_t));
    tcp_conn->fd = client_fd;
    tcp_conn->events = EPOLLIN;
    tcp_conn->loop = listener->loop;
    tcp_conn->on_read =
        handle_read; // Set the read callback for MCU connections
    tcp_conn->on_write = handle_write;
    init_mcu_connection(tcp_conn);

    int unix_fd = create_unix_client();
    set_nonblocking(unix_fd);

    connection_t *unix_conn = calloc(1, sizeof(connection_t));
    unix_conn->fd = unix_fd;
    unix_conn->events = EPOLLIN;
    unix_conn->loop = listener->loop;
    unix_conn->on_read = handle_read;
    unix_conn->on_write = handle_write;
    init_mcu_connection(unix_conn);

    tcp_conn->peer = unix_conn;
    unix_conn->peer = tcp_conn;

    event_loop_add(tcp_conn->loop, tcp_conn->fd, tcp_conn->events, tcp_conn);
    event_loop_add(unix_conn->loop, unix_conn->fd, unix_conn->events, unix_conn);

  }
}

// Entry point of the program that sets up the event loop
/**
 * Main entry point of the program.
 *
 * Initializes the event-driven server using the `epoll` mechanism. Sets up
 * the TCP listener and UNIX domain client connections. Creates the
 * event loop and handles incoming connections and data in real time.
 *
 * @return Always returns 0.
 */
int main() {
 event_loop_t *ev_loop = event_loop_create();

  /* ========== 1. 创建 TCP listener connection ========== */

  connection_t *tcp_conn = calloc(1, sizeof(connection_t));
  tcp_conn->fd = create_tcp_server();
  tcp_conn->loop = ev_loop; 
  // tcp_conn->type = CONN_TCP_LISTENER;
  tcp_conn->on_read = handle_accept; // Set the read callback for accepting new
                                     // connections
  tcp_conn->on_write = NULL;         // No write callback needed for listener
  tcp_conn->in_len = 0;
  tcp_conn->out_len = 0;

  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.ptr = tcp_conn;

event_loop_add(ev_loop, tcp_conn->fd, ev.events, tcp_conn);

event_loop_run(ev_loop); // Start the event loop to process events and callbacks

  return 0; // Exit the program successfully
}
