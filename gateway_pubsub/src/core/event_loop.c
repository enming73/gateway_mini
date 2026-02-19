#include <core/event_loop.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/epoll.h>

struct event_loop {
  int epfd;
  struct epoll_event events[64];
};

event_loop_t *event_loop_create() {
  event_loop_t *loop = calloc(1, sizeof(event_loop_t));
  loop->epfd = epoll_create1(0);
  return loop;
}

void event_loop_add(event_loop_t *loop, int fd, uint32_t events, void *ptr) {
  struct epoll_event ev = {0};
  ev.events = events;
  ev.data.ptr = ptr;
  epoll_ctl(loop->epfd, EPOLL_CTL_ADD, fd, &ev);
}

void event_loop_mod(event_loop_t *loop, int fd, uint32_t events, void *ptr) {
  struct epoll_event ev = {0};
  ev.events = events;
  ev.data.ptr = ptr;
  epoll_ctl(loop->epfd, EPOLL_CTL_MOD, fd, &ev);
}

void event_loop_del(event_loop_t *loop, int fd) {
  epoll_ctl(loop->epfd, EPOLL_CTL_DEL, fd, NULL);
}

void event_loop_run(event_loop_t *loop) {
  while (1) {
    int n = epoll_wait(loop->epfd, loop->events, 64, -1);
    for (int i = 0; i < n; i++) {
      connection_t *conn = loop->events[i].data.ptr;
      uint32_t events = loop->events[i].events;

      if (events & EPOLLIN) {
        conn->on_read(conn);
      }
      if (events & EPOLLOUT) {
        conn->on_write(conn);
      }
    }
  }
}
