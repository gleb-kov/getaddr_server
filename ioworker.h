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

#endif //GETADDR_SERVER_IOWORKER_H
