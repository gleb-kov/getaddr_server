#ifndef GETADDR_SERVER_IOWORKER_H
#define GETADDR_SERVER_IOWORKER_H

#include <sys/epoll.h>
#include <array>
#include <memory>

class TIOWorker {
public:
    TIOWorker();

    void Add(int fd, epoll_event *task);

    void Edit(int fd, epoll_event *task);

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
    explicit TIOTask(TIOWorker *context, uint32_t events, int fd);

    void Callback(uint32_t events);

    ~TIOTask() = default;

    TIOTask(TIOTask const &) = delete;

    TIOTask(TIOTask &&) = delete;

    TIOTask &operator=(TIOTask const &) = delete;

    TIOTask &operator=(TIOTask &&) = delete;

private:
    TIOWorker * context;
    uint32_t events;
    int fd;
    // callback func
};


#endif //GETADDR_SERVER_IOWORKER_H
