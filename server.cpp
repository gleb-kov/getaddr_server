#include "server.h"

TServer::TServer(TIOWorker &io_context, uint32_t address, uint16_t port)
        : Address(address), Port(port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throw std::runtime_error("ERROR: TServer socket() returned some negative number.");
    }

    sockaddr_in sain;
    sain.sin_family = AF_INET;
    sain.sin_addr.s_addr = address;
    sain.sin_port = port;

    int bind_code = bind(fd, reinterpret_cast<sockaddr const *>(&sain), sizeof sain);
    if (bind_code < 0) {
        throw std::runtime_error("ERROR: TServer bind() returned some negative number.");
    }

    int listen_code = listen(fd, SOMAXCONN);
    if (listen_code < 0) {
        throw std::runtime_error("ERROR: TServer listen() returned some negative number.");
    }

    std::function<void(uint32_t)> receiver = [this, &io_context, fd](uint32_t events) {
        // assert (events & EPOLLIN)

        int s = accept4(fd, nullptr, nullptr, SOCK_NONBLOCK);
        if (s < 0) {
            return;
        }

        auto tmp = new TClient(io_context, s, this); // exception ?
        auto t = Connections.insert({tmp, std::unique_ptr<TClient>(tmp)}); // check insertion correctness
    };
    Task = std::make_unique<TIOTask>(&io_context, EPOLLIN, fd, receiver);
}

void TServer::RefuseConnection(TClient *task) {
    Connections.erase(task);
}

TClient::TClient(TIOWorker &io_context, uint32_t s, TServer *server) {
    std::function<void(uint32_t)> echo = [this, s, server](uint32_t events) {
        if ((events & EPOLLERR) || (events & EPOLLRDHUP) || (events & EPOLLHUP)) {
            server->RefuseConnection(this);
            return;
        }
        if (events & EPOLLOUT) {
            send(s, &buf, sizeof buf, 0);
        }
    };
    Task = std::make_unique<TIOTask>(&io_context, CLIENT_EVENTS, s, echo);
}
