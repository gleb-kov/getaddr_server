#include <iostream>

#include "server.h"

int main() {
    TIOWorker io_context;
    TServer server(io_context, htonl(INADDR_ANY), htons(58239));
    io_context.Exec(1000);
    return 0;
}