// Enabling GNU extensions for additional socket and file control functions
#define _GNU_SOURCE
#include "src/core/event_loop.h" // Include the header file for the event loop implementation
#include "src/transport/tcp_listener.h" // Include the header file for utility functions (e.g., set_nonblocking)

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
  transport_tcp_init(ev_loop);

  return 0; // Exit the program successfully
}
