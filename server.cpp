#include "server.h"

TServer::TServer(TIOWorker &io_context, uint32_t address, uint16_t port)
        : Address(address), Port(port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throw std::runtime_error("TServer() socket() call.");
    }

    sockaddr_in sain;
    sain.sin_family = AF_INET;
    sain.sin_addr.s_addr = address;
    sain.sin_port = port;

    int bind_code = bind(fd, reinterpret_cast<sockaddr const *>(&sain), sizeof sain);
    if (bind_code < 0) {
        throw std::runtime_error("TServer() bind() call.");
    }

    int listen_code = listen(fd, SOMAXCONN);
    if (listen_code < 0) {
        throw std::runtime_error("TServer() listen() call.");
    }

    std::function<void(uint32_t, TIOTask *)> receiver =
            [this, &io_context, fd](uint32_t events, TIOTask *self) noexcept(true) {
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

TClient::TClient(TIOWorker &io_context, uint32_t fd, TServer *server) {
    std::function<void(uint32_t, TIOTask *)> echo =
            [this, fd, server, &io_context](uint32_t events, TIOTask *self) noexcept(true) {
                if ((events & EPOLLERR) || (events & EPOLLRDHUP) || (events & EPOLLHUP)) {
                    server->RefuseConnection(this);
                    return;
                }
                if (events & EPOLLIN) {
                    recv(fd, &buf, sizeof buf, 0);
                    epoll_event e{(CLOSE_EVENTS | EPOLLOUT), self};
                    io_context.Edit(fd, &e);
                    return;
                }
                if (events & EPOLLOUT) {
                    send(fd, &buf, sizeof buf, 0);
                    epoll_event e{(CLOSE_EVENTS | EPOLLIN), self};
                    io_context.Edit(fd, &e);
                }
            };
    Task = std::make_unique<TIOTask>(&io_context, (CLOSE_EVENTS | EPOLLIN), fd, echo);
}
