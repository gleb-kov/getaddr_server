#ifndef GETADDR_SERVER_IOJOB_H
#define GETADDR_SERVER_IOJOB_H

#include <array>
#include <cerrno>
// #include <chrono>
#include <csignal>
#include <cstring>
#include <functional>
// #include <queue>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

class TIOWorker {
public:
    TIOWorker();

    void Add(int fd, epoll_event *);

    [[maybe_unused]] void Edit(int fd, epoll_event *);

    [[maybe_unused]] void Remove(int fd, epoll_event *);

    int TryRemove(int fd, epoll_event *) noexcept;

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
    // std::priority_queue<std::chrono::steady_clock::time_point> Actions;
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

#endif //GETADDR_SERVER_IOJOB_H
