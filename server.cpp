#include "server.h"

TServer::TServer(TIOWorker &io_context, uint32_t address, uint16_t port)
        : Address(address), Port(port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throw std::runtime_error(std::string("TServer() socket() call. ") + std::strerror(errno));
    }

    sockaddr_in sain;
    sain.sin_family = AF_INET;
    sain.sin_addr.s_addr = address;
    sain.sin_port = port;

    /* some magic to fix bind() EADDRINUSE on free port */
    int optVal = 1;
    int sockOptCode = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof optVal);
    if (sockOptCode < 0) {
        throw std::runtime_error(std::string("TServer() setsockopt() call. ") + std::strerror(errno));
    }

    int bindCode = bind(fd, reinterpret_cast<sockaddr const *>(&sain), sizeof sain);
    if (bindCode < 0) {
        throw std::runtime_error(std::string("TServer() bind() call. ") + std::strerror(errno));
    }

    int listenCode = listen(fd, SOMAXCONN);
    if (listenCode < 0) {
        throw std::runtime_error(std::string("TServer() listen() call. ") + std::strerror(errno));
    }

    std::function<void(uint32_t, TIOTask *)> receiver =
            [this, &io_context, fd](uint32_t events, TIOTask *self) noexcept(true) {
                if (events != EPOLLIN) {
                    return;
                }

                int sfd = accept4(fd, nullptr, nullptr, SOCK_NONBLOCK);
                if (sfd < 0) {
                    return;
                }

                TClient *clientPtr = nullptr;
                try {
                    clientPtr = new TClient(io_context, sfd, this);
                    Connections.insert({clientPtr, std::unique_ptr<TClient>(clientPtr)});
                } catch (...) {
                    delete clientPtr;
                }
            };
    Task = std::make_unique<TIOTask>(&io_context, EPOLLIN, fd, receiver);
}

void TServer::RefuseConnection(TClient *task) {
    Connections.erase(task);
}

TClient::TClient(TIOWorker &io_context, uint32_t fd, TServer *server) : Server(server) {
    std::function<void(uint32_t, TIOTask *)> echo =
            [this, fd, &io_context](uint32_t events, TIOTask *self) noexcept(true) {
                if ((events & EPOLLERR) || (events & EPOLLRDHUP) || (events & EPOLLHUP)) {
                    Finish();
                    return;
                }
                if (events & EPOLLIN) {
                    int code = recv(fd, &Buffer, DOMAIN_MAX_LENGTH, 0);
                    if (code < 0) {
                        Finish();
                    } else {
                        Queries.push({Buffer, code});
                        epoll_event e{(CLOSE_EVENTS | EPOLLOUT), self};
                        io_context.Edit(fd, &e);
                    }
                    return;
                }
                if (events & EPOLLOUT) {
                    int code = send(fd, Queries.front().first.c_str(), Queries.front().second, 0);
                    Queries.pop();

                    if (code < 0) {
                        Finish();
                    } else {
                        epoll_event e{(CLOSE_EVENTS | EPOLLIN), self};
                        io_context.Edit(fd, &e);
                    }
                }
            };
    Task = std::make_unique<TIOTask>(&io_context, (CLOSE_EVENTS | EPOLLIN), fd, echo);
}

void TClient::Finish() {
    Server->RefuseConnection(this);
}
