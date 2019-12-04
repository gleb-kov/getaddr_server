#ifndef GETADDR_SERVER_GETADDRINFO_TASK_H
#define GETADDR_SERVER_GETADDRINFO_TASK_H

#include <iostream>
#include <netdb.h>
#include <arpa/inet.h>

class TGetaddrinfoTask {
public:
    TGetaddrinfoTask();

    int Ask(const char *);

    ~TGetaddrinfoTask() = default;

    TGetaddrinfoTask(TGetaddrinfoTask const &) = delete;

    TGetaddrinfoTask(TGetaddrinfoTask &&) = delete;

    TGetaddrinfoTask &operator=(TGetaddrinfoTask const &) = delete;

    TGetaddrinfoTask &operator=(TGetaddrinfoTask &&) = delete;

private:
    addrinfo Hints;
    addrinfo *Result;
    addrinfo *Node;
    int ErrorCode;
    char addrstr[100];
    void *ptr;
};

#endif //GETADDR_SERVER_GETADDRINFO_TASK_H
