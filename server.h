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
public:
    using time_point = std::chrono::steady_clock::time_point;

private:
    struct TClientTimer {
        explicit TClientTimer(int64_t timeout);

        void AddClient(std::unique_ptr<TClient> &client);

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

    void ConnectClient(std::unique_ptr<TClient> &client);

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
    using time_point = TIOWorker::time_point;
    using callback_t = std::function<void(TIOTask * const, uint32_t)>;

    TIOTask(TIOWorker *context,
            int fd,
            callback_t &callback,
            std::function<void()> &finisher,
            uint32_t events);

    [[deprecated]] void SetTask(callback_t &callback);

    /* TClient api */
    void Reconfigure(uint32_t other = 0);

    [[nodiscard]] int Read(char *, size_t);

    int Write(const char *, size_t);

    void Close();

    /* TIOWorker api */
    void Callback(uint32_t events) noexcept;

    time_point GetLastTime() const;

    ~TIOTask();

    TIOTask(TIOTask const &) = delete;

    TIOTask(TIOTask &&) = delete;

    TIOTask &operator=(TIOTask const &) = delete;

    TIOTask &operator=(TIOTask &&) = delete;

    static bool IsClosingEvent(uint32_t event);

private:
    void UpdateTime();

public:
    static constexpr uint32_t CLOSE_EVENTS =
            (EPOLLERR | EPOLLRDHUP | EPOLLHUP);

private:
    bool Destroy = false;
    TIOWorker *const Context;
    const int fd;
    callback_t CallbackHandler;
    time_point LastAction;
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
public:
    using time_point = TIOWorker::time_point;

    TClient(TIOWorker *io_context, int fd, uint32_t startEvents);

    time_point GetLastTime() const;

    ~TClient() = default;

    TClient(TClient const &) = delete;

    TClient(TClient &&) = delete;

    TClient &operator=(TClient const &) = delete;

    TClient &operator=(TClient &&) = delete;

private:
    char Buffer[TGetaddrinfoTask::QUERY_MAX_LENGTH] = {0};
    TGetaddrinfoTask QueryProcessor;
    std::unique_ptr<TIOTask> Task;
};

#endif //GETADDR_SERVER_SERVER_H
