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
        TIOWorker io_context(500);
        TServer server(io_context, htonl(INADDR_ANY), htons(port));
        io_context.Exec();
    } catch (std::exception const &e) {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "UNKNOWN EXCEPTION." << std::endl;
    }

    /*TGetaddrinfoTask solver;
    int err1 = solver.Ask("example.com");
    int err2 = solver.Ask("vk.com");
    int err3 = solver.Ask("google.com");
    int err4 = solver.Ask("abracadabra");*/
    return 0;
}
