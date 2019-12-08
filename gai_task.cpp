#include "gai_task.h"

TGetaddrinfoTask::TGetaddrinfoTask()
        : Hints{0, AF_UNSPEC, SOCK_STREAM}, Info(nullptr), Node(nullptr) {}

int TGetaddrinfoTask::Ask(const char *host) {
    int errorCode = getaddrinfo(host, nullptr, &Hints, &Info);

    std::cerr << "Host: " << host << std::endl;
    if (errorCode != 0) {
        std::cerr << "Getaddrinfo() failed. Error: " << errorCode << std::endl;
        return errorCode;
    }

    void *ptr = nullptr;
    for (Node = Info; Node; Node = Node->ai_next) {
        if (Node->ai_family == AF_INET) {
            ptr = &(reinterpret_cast<sockaddr_in *>(Node->ai_addr)->sin_addr);
        } else if (Node->ai_family == AF_INET6) {
            ptr = &(reinterpret_cast<sockaddr_in6 *>(Node->ai_addr)->sin6_addr);
        } else {
            continue;
        }

        inet_ntop(Node->ai_family, ptr, addrstr, IP_NODE_MAX_SIZE);
        std::cerr << addrstr << std::endl;
    }
    freeaddrinfo(Info);
    return 0;
}
