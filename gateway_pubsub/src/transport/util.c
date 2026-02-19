#include <fcntl.h>

/**
 * Sets the provided file descriptor to non-blocking mode.
 *
 * @param fd - The file descriptor to modify.
 * @return 0 on success, or -1 if an error occurs.
 */
int set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
