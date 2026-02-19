#ifndef MCU_PROTOCOL_H
#define MCU_PROTOCOL_H
#include <core/connection.h>

void handle_mcu_read(connection_t *conn);
void handle_write(connection_t *conn);

#endif // MCU_PROTOCOL_H
