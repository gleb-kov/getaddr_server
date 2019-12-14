#ifndef GETADDR_SERVER_SERVER_H
#define GETADDR_SERVER_SERVER_H

#include <array>
#include <cerrno>
#include <chrono>
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

#include "gai_task.h"

class TClient;

class TIOWorker {
    class TClientTimer {
        using time_point = std::chrono::steady_clock::time_point;

    public:
        explicit TClientTimer(int64_t timeout);

        void AddClient(TClient *client);

        void RefuseClient(TClient *client);

        [[maybe_unused]] int64_t NextCheck();

        void RemoveOld();

        ~TClientTimer() = default;

        TClientTimer(TClientTimer const &) = delete;

        TClientTimer(TClientTimer &&) = delete;

        TClientTimer &operator=(TClientTimer const &) = delete;

        TClientTimer &operator=(TClientTimer &&) = delete;

    private:
        [[nodiscard]] int64_t TimeDiff(time_point const &lhs, time_point const &rhs) const;

    private:
        const int64_t TimeOut;
        std::unordered_map<TClient *, time_point> CachedAction;
        std::map<std::pair<time_point, TClient *>,
                std::unique_ptr<TClient>> Connections, Fake;
    };
public:
    explicit TIOWorker(int64_t sockTimeout = 600);

    void ConnectClient(TClient *client);

    void RefuseClient(TClient *client);

    void Add(int fd, epoll_event *);

    [[maybe_unused]] void Edit(int fd, epoll_event *);

    [[maybe_unused]] void Remove(int fd, epoll_event *);

    [[maybe_unused]] int TryRemove(int fd, epoll_event *) noexcept;

    void Exec(int64_t epollTimeout = -1);

    ~TIOWorker() = default;

    TIOWorker(TIOWorker const &) = delete;

    TIOWorker(TIOWorker &&) = delete;

    TIOWorker &operator=(TIOWorker const &) = delete;

    TIOWorker &operator=(TIOWorker &&) = delete;

public:
    static const size_t TIOWORKER_EPOLL_MAX = 2000;

private:
    int efd;
    TClientTimer Clients;
};

class TIOTask {
public:
    TIOTask(TIOWorker *context,
            int fd,
            std::function<void(uint32_t)> &callback,
            std::function<void()> &finisher,
            uint32_t events);

    void Reconfigure(bool in, bool out, uint32_t other = 0);

    [[maybe_unused]] int Read(char *, size_t);

    [[maybe_unused]] void Write(const char *, size_t);

    void Callback(uint32_t events) noexcept;

    ~TIOTask();

    TIOTask(TIOTask const &) = delete;

    TIOTask(TIOTask &&) = delete;

    TIOTask &operator=(TIOTask const &) = delete;

    TIOTask &operator=(TIOTask &&) = delete;

    static bool IsClosingEvent(uint32_t event);

public:
    static constexpr uint32_t CLOSE_EVENTS =
            (EPOLLERR | EPOLLRDHUP | EPOLLHUP);

private:
    TIOWorker *const Context;
    const int fd;
    std::function<void(uint32_t)> CallbackHandler;
    std::function<void()> FinishHandler;
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
    using time_point = std::chrono::steady_clock::time_point;

public:
    TClient(TIOWorker *io_context, int fd);

    time_point GetLastTime() const;

    void Configure();

    void Finish();

    ~TClient() = default;

    TClient(TClient const &) = delete;

    TClient(TClient &&) = delete;

    TClient &operator=(TClient const &) = delete;

    TClient &operator=(TClient &&) = delete;

private:
    static const size_t DOMAIN_MAX_LENGTH = 255;

    char Buffer[DOMAIN_MAX_LENGTH] = {0};
    time_point LastAction;
    TIOWorker *const Context;
    TGetaddrinfoTask QueryProcesser;
    std::unique_ptr<TIOTask> Task;
};

#endif //GETADDR_SERVER_SERVER_H
