#include "gai_task.h"

TGetaddrinfoTask::TGetaddrinfoTask()
        : hints{0, AF_UNSPEC, SOCK_STREAM}, res(nullptr), node(nullptr), ptr(nullptr) {}

int TGetaddrinfoTask::ask(const char *host) {
    errcode = getaddrinfo(host, nullptr, &hints, &res);

    std::cerr << "Host: " << host << std::endl;
    if (errcode != 0) {
        std::cerr << "Getaddrinfo() failed. Errcode: " << errcode;
        return errcode;
    }

    node = res;
    while (node) {
        inet_ntop(node->ai_family, node->ai_addr->sa_data, addrstr, 100);

        switch (node->ai_family) {
            [[likely]] case AF_INET:
                ptr = &((sockaddr_in *) node->ai_addr)->sin_addr;
                break;
            case AF_INET6:
                ptr = &((sockaddr_in6 *) node->ai_addr)->sin6_addr;
                break;
        }
        inet_ntop(node->ai_family, ptr, addrstr, 100);
        std::cerr << "IPv"
                  << (node->ai_family == PF_INET6 ? 6 : 4)
                  << ": "
                  << addrstr
                  << std::endl;
        node = node->ai_next;
    }
    freeaddrinfo(res);
    return 0;
}
