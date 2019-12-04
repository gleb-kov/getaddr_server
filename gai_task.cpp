#include "gai_task.h"

TGetaddrinfoTask::TGetaddrinfoTask()
        : Hints{0, AF_UNSPEC, SOCK_STREAM}, Result(nullptr), Node(nullptr), ptr(nullptr) {}

int TGetaddrinfoTask::Ask(const char *host) {
    int errorCode = getaddrinfo(host, nullptr, &Hints, &Result);

    std::cerr << "Host: " << host << std::endl;
    if (errorCode != 0) {
        std::cerr << "Getaddrinfo() failed. Error: " << errorCode << std::endl;
        return errorCode;
    }

    for (Node = Result; Node; Node = Node->ai_next) {
        if (Node->ai_family == AF_INET) {
            ptr = &((sockaddr_in *) Node->ai_addr)->sin_addr;
        } else if (Node->ai_family == AF_INET6) {
            ptr = &((sockaddr_in6 *) Node->ai_addr)->sin6_addr;
        } else {
            continue;
        }

        inet_ntop(Node->ai_family, ptr, addrstr, sizeof addrstr);
        std::cerr << "IPv"
                  << (Node->ai_family == AF_INET6 ? 6 : 4)
                  << ": "
                  << addrstr
                  << std::endl;
    }
    freeaddrinfo(Result);
    return 0;
}
