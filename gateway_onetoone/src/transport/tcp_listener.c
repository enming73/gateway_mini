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
#include "../core/event_loop.h" // Include the header file for the event loop implementation
#include "../protocol/mcu_protocol.h" // Include the header file for MCU protocol handling (e.g., handle_read, handle_write)
#include "util.h" // Include the header file for utility functions (e.g., set_nonblocking)
#include <unistd.h>

#define TCP_PORT 9000
#define UNIX_SOCKET_PATH "/tmp/gateway.sock"

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

    connection_t *tcp_conn = connection_create(listener->loop, client_fd);
    tcp_conn->on_read =
        handle_read; // Set the read callback for MCU connections
    tcp_conn->on_write = handle_write;

    int unix_fd = create_unix_client();
    set_nonblocking(unix_fd);

    connection_t *unix_conn = connection_create(listener->loop, unix_fd);
    unix_conn->on_read = handle_read;
    unix_conn->on_write = handle_write;

    tcp_conn->peer = unix_conn;
    unix_conn->peer = tcp_conn;

    event_loop_add(tcp_conn->loop, tcp_conn->fd, tcp_conn->events, tcp_conn);
    event_loop_add(unix_conn->loop, unix_conn->fd, unix_conn->events,
                   unix_conn);
  }
}

void transport_tcp_init(event_loop_t *loop) {
  connection_t *tcp_conn = calloc(1, sizeof(connection_t));
  tcp_conn->fd = create_tcp_server();
  tcp_conn->loop = loop;
  tcp_conn->on_read =
      handle_accept;         // Set the accept callback for the TCP listener
  tcp_conn->on_write = NULL; // No write callback needed for the listener

  tcp_conn->in_len = 0;
  tcp_conn->out_len = 0;
  tcp_conn->events = EPOLLIN; // Listen for incoming connections (read events)
  event_loop_add(loop, tcp_conn->fd, tcp_conn->events, tcp_conn);
  event_loop_run(loop); // Start the event loop to process events and callbacks
}
