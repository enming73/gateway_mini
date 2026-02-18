#include <stdint.h>

#include "connection.h"

event_loop_t *event_loop_create();
void event_loop_run(event_loop_t *loop);

void event_loop_add(event_loop_t *loop, int fd, uint32_t events, void *ptr);
void event_loop_mod(event_loop_t *loop, int fd, uint32_t events, void *ptr);
void event_loop_del(event_loop_t *loop, int fd);
