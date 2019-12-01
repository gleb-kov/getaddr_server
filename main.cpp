#include <iostream>

#include "server.h"

// TODO:
//  check connection closing in iotask destructor
//  exceptions
//  check signals handlers, mltpl servers
//  const, references, attributes, moves
//  choose optimal data structures: array, map, unique_ptr
//  self-editing getaddrinfo task as lambda
//  codestyle

int main() {
    TIOWorker io_context;
    TServer server(io_context, htonl(INADDR_ANY), htons(58239));
    io_context.Exec(1000);
    return 0;
}