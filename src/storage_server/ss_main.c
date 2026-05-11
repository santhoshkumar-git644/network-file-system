#include "common.h"
#include "logger.h"

int main(int argc, char *argv[]) {
    init_logger("ss.log");
    log_message(LOG_INFO, "Storage Server started.");

    // Parse arguments (e.g., port to listen on)
    int ss_port = 8081; // default fallback
    if (argc > 1) {
        ss_port = atoi(argv[1]);
    }

    log_message(LOG_INFO, "Storage Server will listen on port %d", ss_port);

    // Main loop
    while (1) {
        sleep(1);
    }

    close_logger();
    return 0;
}
