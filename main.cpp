#include <iostream>

#include "server.h"

int main() {
    TIOWorker io_context;
    TServer(io_context, htonl(INADDR_ANY), 1539);
    io_context.Exec();
    return 0;
}