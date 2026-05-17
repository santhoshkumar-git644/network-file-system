#ifndef SS_CLIENT_HANDLER_H
#define SS_CLIENT_HANDLER_H

#include "common.h"
#include "client_handler.h" // For ClientCommand structure

void* ss_client_listener_thread(void* arg);
void* ss_handle_client_connection(void* arg);

#endif // SS_CLIENT_HANDLER_H
