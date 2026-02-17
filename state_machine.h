#include "connection.h"

void update_events(connection_t *conn);
void disable_read(connection_t *conn);
void disable_write(connection_t *conn);
void enable_write(connection_t *conn);
void shutdown_write(connection_t *conn);

void connection_on_read_eof(connection_t *conn);
void connection_maybe_close(connection_t *conn);
