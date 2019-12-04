#ifndef GETADDR_SERVER_GETADDRINFO_TASK_H
#define GETADDR_SERVER_GETADDRINFO_TASK_H

#include <iostream>
#include <netdb.h>
#include <arpa/inet.h>

class TGetaddrinfoTask {
public:
    TGetaddrinfoTask();

    int ask(const char *);

    ~TGetaddrinfoTask() = default;

private:
    addrinfo hints;
    addrinfo *res;
    addrinfo *node;
    int errcode;
    char addrstr[100];
    void *ptr;
};

#endif //GETADDR_SERVER_GETADDRINFO_TASK_H
