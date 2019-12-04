#include <iostream>

#include "server.h"
#include "gai_task.h"

int main(int argc, char *argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: ./getaddr-server <port> (default 58239)" << std::endl;
        return 1;
    }

    int port = (argc == 2 ? atoi(argv[1]) : 58239);

    try {
        TIOWorker io_context;
        TServer server(io_context, htonl(INADDR_ANY), htons(port));
        io_context.Exec(500);
    } catch (std::exception const &e) {
        std::cerr << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception." << std::endl;
    }

    /*
     TGetaddrinfoTask solver;
     int err1 = solver.ask("example.com");
     int err2 = solver.ask("vk.com");
     */
    return 0;
}
