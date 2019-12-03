#ifndef GETADDR_SERVER_SERVER_H
#define GETADDR_SERVER_SERVER_H

#include <memory>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unordered_map>

#include "iojob.h"

class TClient;

class TServer {
public:
    TServer(TIOWorker &io_context, uint32_t address, uint16_t port);

    void RefuseConnection(TClient *task);

    ~TServer() = default;

    TServer(TServer const &) = delete;

    TServer(TServer &&) = delete;

    TServer &operator=(TServer const &) = delete;

    TServer &operator=(TServer &&) = delete;

private:
    const uint32_t Address;
    const uint16_t Port;

    std::unique_ptr<TIOTask> Task;
    std::unordered_map<TClient *, std::unique_ptr<TClient>> Connections;
};

class TClient {
public:
    TClient(TIOWorker &io_context, uint32_t s, TServer *server);

    ~TClient() = default;

    TClient(TClient const &) = delete;

    TClient(TClient &&) = delete;

    TClient &operator=(TClient const &) = delete;

    TClient &operator=(TClient &&) = delete;

private:
    static constexpr uint32_t CLOSE_EVENTS = (EPOLLERR | EPOLLRDHUP | EPOLLHUP);

    char buf[20];
    std::unique_ptr<TIOTask> Task;
};

#endif //GETADDR_SERVER_SERVER_H
