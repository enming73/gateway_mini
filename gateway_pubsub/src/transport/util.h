#ifndef TRANSPORT_UTIL_H
#define TRANSPORT_UTIL_H
/**
 * Sets the provided file descriptor to non-blocking mode.
 *
 * @param fd - The file descriptor to modify.
 * @return 0 on success, or -1 if an error occurs.
 */
int set_nonblocking(int fd);

#endif // TRANSPORT_UTIL_H
