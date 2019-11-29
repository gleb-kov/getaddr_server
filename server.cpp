#include "server.h"

TServer::TServer(TIOWorker &io_context, uint32_t address, uint16_t port)
        : Address(address), Port(port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) {
        throw std::runtime_error("ERROR: TServer socket() returned some negative number");
    }

    sockaddr_in sain;
    sain.sin_family = AF_INET;
    sain.sin_addr.s_addr = address;
    sain.sin_port = port;

    int bind_code = bind(fd, reinterpret_cast<sockaddr const *>(&sain), sizeof sain);

    if (bind_code < 0) {
        throw std::runtime_error("ERROR: TServer bind() returned some negative number");
    }

    int listen_code = listen(fd, SOMAXCONN);

    if (listen_code < 0) {
        throw std::runtime_error("ERROR: TServer listen() returned some negative number");
    }

    auto tmp = new TIOTask(&io_context, EPOLLIN, fd, [this, &io_context, fd](uint32_t events) {
        int s = accept4(fd, nullptr, nullptr, SOCK_NONBLOCK);
        if (s < 0) return;

        auto child = new TIOTask(&io_context, (EPOLLIN | EPOLLOUT), s, [s](uint32_t events) {
            char buf[20] = "hi there";
            if (events & EPOLLOUT) {
                send(s, &buf, sizeof buf, 0);
            }
        });
        Connections.insert({child, std::unique_ptr<TIOTask>(child)});
    });
    // Task = std::unique_ptr<TIOTask>(tmp);
}
