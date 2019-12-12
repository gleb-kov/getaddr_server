#include <iostream>

#include "server.h"

int main(int argc, char *argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: ./getaddr-server <port> (default 58239)" << std::endl;
        return 1;
    }

    int port = (argc == 2 ? atoi(argv[1]) : 58239);

    try {
        TIOWorker io_context(5);
        TServer server(io_context, htonl(INADDR_ANY), htons(port));
        io_context.Exec(1000);
    } catch (std::exception const &e) {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "UNKNOWN EXCEPTION." << std::endl;
    }
    return 0;
}
