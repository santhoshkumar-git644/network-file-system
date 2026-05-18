#include "common.h"
#include "logger.h"
#include "ss_core.h"
#include "ss_client_handler.h"

int main(int argc, char *argv[]) {
    init_logger("ss.log");
    log_message(LOG_INFO, "Storage Server started.");

    // Parse arguments (e.g., port to listen on)
    int ss_port = 8081; // default fallback
    if (argc > 1) {
        ss_port = atoi(argv[1]);
    }

    log_message(LOG_INFO, "Storage Server will listen on port %d", ss_port);
    
    int* client_port_arg = malloc(sizeof(int));
    *client_port_arg = ss_port + 100; // SS Client port is NM port + 100
    
    pthread_t client_thread;
    pthread_create(&client_thread, NULL, ss_client_listener_thread, client_port_arg);
    
    // Connect and register with NM
    ss_init(*client_port_arg);

    // Main loop
    while (1) {
        sleep(1);
    }

    close_logger();
    return 0;
}
