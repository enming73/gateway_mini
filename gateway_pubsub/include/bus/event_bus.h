#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include <core/connection.h>

void event_bus_init();
void event_subscribe(const char *topic, connection_t *conn);
void event_unsubscribe_all(connection_t *conn);
void event_publish(const char *topic, const char *data, int len);

#endif
