#ifndef GETADDR_SERVER_SERVER_H
#define GETADDR_SERVER_SERVER_H

#include <array>
#include <cerrno>
// #include <chrono>
#include <csignal>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <netinet/in.h>
#include <queue>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

class TClient;

class TClientTimer {
public:
    explicit TClientTimer(int timeout);

    void AddClient(TClient *client);

    void RefuseClient(TClient *client);

    int NextCheck();

    void RemoveOld();

private:
    int TimeOut;
    std::unordered_map<TClient *, int> CachedAction;
    std::map<std::pair<int, TClient*>, std::unique_ptr<TClient>> Connections, FakeConnections;
};

class TIOWorker {
public:
    explicit TIOWorker(int timeout = -1);

    void ConnectClient(TClient *client);

    void RefuseClient(TClient *client);

    void Add(int fd, epoll_event *);

    [[maybe_unused]] void Edit(int fd, epoll_event *);

    [[maybe_unused]] void Remove(int fd, epoll_event *);

    int TryRemove(int fd, epoll_event *) noexcept;

    void Exec();

    ~TIOWorker() = default;

    TIOWorker(TIOWorker const &) = delete;

    TIOWorker(TIOWorker &&) = delete;

    TIOWorker &operator=(TIOWorker const &) = delete;

    TIOWorker &operator=(TIOWorker &&) = delete;

public:
    static const size_t TIOWORKER_EPOLL_MAX = 10000;

private:
    int efd;
    TClientTimer Clients;
};

class TIOTask {
public:
    TIOTask(TIOWorker *context,
            uint32_t events,
            int fd,
            std::function<void(uint32_t, TIOTask *)> callback);

    void Callback(uint32_t events) noexcept;

    ~TIOTask();

    TIOTask(TIOTask const &) = delete;

    TIOTask(TIOTask &&) = delete;

    TIOTask &operator=(TIOTask const &) = delete;

    TIOTask &operator=(TIOTask &&) = delete;

private:
    TIOWorker *Context;
    uint32_t Events;
    int fd;
    std::function<void(uint32_t, TIOTask *)> CallbackHandler;
};

class TServer {
public:
    TServer(TIOWorker &io_context, uint32_t address, uint16_t port);

    ~TServer() = default;

    TServer(TServer const &) = delete;

    TServer(TServer &&) = delete;

    TServer &operator=(TServer const &) = delete;

    TServer &operator=(TServer &&) = delete;

private:
    const uint32_t Address;
    const uint16_t Port;

    std::unique_ptr<TIOTask> Task;
};

class TClient {
public:
    TClient(TIOWorker *io_context, int fd);

    void Finish();

    ~TClient() = default;

    TClient(TClient const &) = delete;

    TClient(TClient &&) = delete;

    TClient &operator=(TClient const &) = delete;

    TClient &operator=(TClient &&) = delete;

private:
    static const size_t DOMAIN_MAX_LENGTH = 255;
    static constexpr uint32_t CLOSE_EVENTS = (EPOLLERR | EPOLLRDHUP | EPOLLHUP);

    // may help in future with mltthreading or replace with just pair
    std::queue<std::pair<std::string, size_t>> Queries;
    char Buffer[DOMAIN_MAX_LENGTH];

    TIOWorker *Context;
    std::unique_ptr<TIOTask> Task;
};

#endif //GETADDR_SERVER_SERVER_H
