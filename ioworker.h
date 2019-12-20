#ifndef GETADDR_SERVER_IOWORKER_H
#define GETADDR_SERVER_IOWORKER_H

#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <sys/epoll.h>
#include <unordered_map>

class TIOTask;

class TIOWorker {
public:
    using time_point = std::chrono::steady_clock::time_point;

private:
    struct TTimer {
        explicit TTimer(int64_t timeout);

        void AddClient(std::unique_ptr<TIOTask> &client);

        void RefuseClient(TIOTask *client);

        [[deprecated]] int64_t NextCheck();

        void RemoveOld();

        ~TTimer() = default;

        TTimer(TTimer const &) = delete;

        TTimer(TTimer &&) = delete;

        TTimer &operator=(TTimer const &) = delete;

        TTimer &operator=(TTimer &&) = delete;

    private:
        [[nodiscard]] int64_t TimeDiff(time_point const &lhs, time_point const &rhs) const;

    private:
        const int64_t TimeOut;
        std::unordered_map<TIOTask *, time_point> CachedAction;
        std::map<std::pair<time_point, TIOTask *>,
                std::unique_ptr<TIOTask>> Connections, Fake;
    };

public:
    explicit TIOWorker(int64_t sockTimeout = 600);

    void ConnectClient(std::unique_ptr<TIOTask> &client);

    void RefuseClient(TIOTask *client);

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
    TTimer Clients;
};

#endif //GETADDR_SERVER_IOWORKER_H
