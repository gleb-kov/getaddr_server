#ifndef GETADDR_SERVER_IOWORKER_H
#define GETADDR_SERVER_IOWORKER_H

#include <array>
#include <csignal>
#include <functional>
#include <memory>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

class TIOWorker {
public:
    TIOWorker() noexcept(false);

    void Add(int fd, epoll_event *task);

    [[maybe_unused]] void Edit(int fd, epoll_event *task);

    void Remove(int fd, epoll_event *task);

    void Exec(int timeout = -1);

    ~TIOWorker() = default;

    TIOWorker(TIOWorker const &) = delete;

    TIOWorker(TIOWorker &&) = delete;

    TIOWorker &operator=(TIOWorker const &) = delete;

    TIOWorker &operator=(TIOWorker &&) = delete;

public:
    static const size_t TIOWORKER_EPOLL_MAX = 10000;

private:
    int efd;
};

class TIOTask {
public:
    TIOTask(TIOWorker *context, uint32_t events, int fd, std::function<void(uint32_t)> callback);

    void Callback(uint32_t events);

    ~TIOTask();

    TIOTask(TIOTask const &) = delete;

    TIOTask(TIOTask &&) = delete;

    TIOTask &operator=(TIOTask const &) = delete;

    TIOTask &operator=(TIOTask &&) = delete;

private:
    TIOWorker *Context;
    uint32_t Events;
    int fd;
    std::function<void(uint32_t)> CallbackHandler;
};

#endif //GETADDR_SERVER_IOWORKER_H
