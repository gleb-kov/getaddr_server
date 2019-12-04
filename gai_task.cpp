#include "gai_task.h"

TGetaddrinfoTask::TGetaddrinfoTask()
        : Hints{0, AF_UNSPEC, SOCK_STREAM}, Result(nullptr), Node(nullptr), ErrorCode(0), ptr(nullptr) {}

int TGetaddrinfoTask::Ask(const char *host) {
    ErrorCode = getaddrinfo(host, nullptr, &Hints, &Result);

    std::cerr << "Host: " << host << std::endl;
    if (ErrorCode != 0) {
        std::cerr << "Getaddrinfo() failed. Error: " << ErrorCode;
        return ErrorCode;
    }

    Node = Result;
    while (Node) {
        switch (Node->ai_family) {
            [[likely]] case AF_INET:
                ptr = &((sockaddr_in *) Node->ai_addr)->sin_addr;
                break;
            case AF_INET6:
                ptr = &((sockaddr_in6 *) Node->ai_addr)->sin6_addr;
                break;
        }
        inet_ntop(Node->ai_family, ptr, addrstr, sizeof addrstr);
        std::cerr << "IPv"
                  << (Node->ai_family == AF_INET6 ? 6 : 4)
                  << ": "
                  << addrstr
                  << std::endl;
        Node = Node->ai_next;
    }
    freeaddrinfo(Result);
    return 0;
}
