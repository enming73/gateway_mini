// Enabling GNU extensions for additional socket and file control functions
#define _GNU_SOURCE
#include <core/event_loop.h>
#include <transport/tcp_listener.h>
#include <transport/unix_listener.h>

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

  transport_unix_init(ev_loop);

  event_loop_run(ev_loop);

  return 0; // Exit the program successfully
}
