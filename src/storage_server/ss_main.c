#include "common.h"
#include "logger.h"
#include "ss_core.h"
#include "ss_client_handler.h"
#include "ss_file_manager.h"

int main(int argc, char *argv[]) {
    init_logger("ss.log");
    log_message(LOG_INFO, "Starting Storage Server...");
    
    init_file_manager();
    
    // Usage: ./storage_server [nm_ip] [ss_port] [client_port] [own_ip]
    const char* nm_ip = "127.0.0.1";
    int ss_port = 8080;
    int client_port = 8180;
    
    if (argc > 1) nm_ip     = argv[1];
    if (argc > 2) ss_port   = atoi(argv[2]);
    if (argc > 3) client_port = atoi(argv[3]);
    
    const char* own_ip = nm_ip; // default to same as NM IP
    if (argc > 4) own_ip = argv[4];

    log_message(LOG_INFO, "Storage Server will listen on client port %d, connect to NM at %s:%d, registering as IP %s",
                client_port, nm_ip, ss_port, own_ip);
    
    int* client_port_arg = malloc(sizeof(int));
    *client_port_arg = client_port;
    
    pthread_t client_thread;
    pthread_create(&client_thread, NULL, ss_client_listener_thread, client_port_arg);
    
    // Connect and register with NM
    connect_to_nm(nm_ip, ss_port, client_port, own_ip);

    // Main loop
    while (1) {
        sleep(1);
    }

    close_logger();
    return 0;
}
