#ifndef GETADDR_SERVER_SERVER_H
#define GETADDR_SERVER_SERVER_H

#include <cerrno>
// #include <chrono>
#include <cstring>
#include <functional>
#include <memory>
#include <netinet/in.h>
#include <queue>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unordered_map>

#include "iojob.h"

class TClient;

class TServer {
public:
    TServer(TIOWorker &io_context, uint32_t address, uint16_t port);

    void RefuseConnection(TClient *);

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
    TClient(TIOWorker &io_context, uint32_t fd, TServer *);

    ~TClient() = default;

    TClient(TClient const &) = delete;

    TClient(TClient &&) = delete;

    TClient &operator=(TClient const &) = delete;

    TClient &operator=(TClient &&) = delete;

private:
    void Finish();

private:
    static const size_t DOMAIN_MAX_LENGTH = 255;
    static constexpr uint32_t CLOSE_EVENTS = (EPOLLERR | EPOLLRDHUP | EPOLLHUP);

    // std::chrono::steady_clock::time_point LastAction;

    // may help in future with mltthreading or replace with just pair
    std::queue<std::pair<std::string, size_t>> Queries;
    char Buffer[DOMAIN_MAX_LENGTH];

    TServer *Server;
    std::unique_ptr<TIOTask> Task;
};

#endif //GETADDR_SERVER_SERVER_H
