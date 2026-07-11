#include "common.h"
#include "logger.h"
#include "ss_core.h"
#include "ss_client_handler.h"
#include "ss_file_manager.h"

int main(int argc, char *argv[]) {
    init_logger("ss.log");
    log_message(LOG_INFO, "Starting Storage Server...");
    
    init_file_manager();
    
    // Fix #17: Accept port and NM IP as arguments
    // Usage: ./storage_server [nm_ip] [ss_port] [client_port]
    const char* nm_ip = "127.0.0.1";
    int ss_port = 8080;
    int client_port = 8180; // Fix #17: separate explicit client port instead of ss_port+100
    
    if (argc > 1) nm_ip     = argv[1];
    if (argc > 2) ss_port   = atoi(argv[2]);
    if (argc > 3) client_port = atoi(argv[3]);

    log_message(LOG_INFO, "Storage Server will listen on client port %d, connect to NM at %s:%d",
                client_port, nm_ip, ss_port);
    
    int* client_port_arg = malloc(sizeof(int));
    *client_port_arg = client_port;
    
    pthread_t client_thread;
    pthread_create(&client_thread, NULL, ss_client_listener_thread, client_port_arg);
    
    // Connect and register with NM
    // Fix #16: pass nm_ip instead of hardcoded string
    connect_to_nm(nm_ip, ss_port, client_port);

    // Main loop
    while (1) {
        sleep(1);
    }

    close_logger();
    return 0;
}
