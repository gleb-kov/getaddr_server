#ifndef GETADDR_SERVER_GETADDRINFO_TASK_H
#define GETADDR_SERVER_GETADDRINFO_TASK_H

#include <arpa/inet.h>
#include <iostream>
#include <netdb.h>

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
    static const size_t IP_NODE_MAX_SIZE = 46;

    addrinfo Hints;
    addrinfo *Info;
    addrinfo *Node;
    char addrstr[IP_NODE_MAX_SIZE];
};

#endif //GETADDR_SERVER_GETADDRINFO_TASK_H
