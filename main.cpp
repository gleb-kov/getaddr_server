#include <iostream>

#include "server.h"

// TODO:
//  self-editing getaddrinfo task as lambda
//  exceptions

int main(int argc, char *argv[]) {
    if (argc > 2) {
        std::cerr << "Usage: ./getaddr-server <port> (default 58239)\n";
        return 0;
    }

    int port = 58239;
    if (argc == 2) {
        port = atoi(argv[1]);
    }


    TIOWorker io_context;
    TServer server(io_context, htonl(INADDR_ANY), htons(port));
    io_context.Exec(500);
    return 0;
}