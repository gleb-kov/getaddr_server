#ifndef GETADDR_SERVER_IOTASK_H
#define GETADDR_SERVER_IOTASK_H

#include <chrono>
#include <functional>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ioworker.h"

class TIOTask {
public:
    using time_point = TIOWorker::time_point;
    using callback_t = std::function<void(TIOTask * const, uint32_t)>;
    using finish_t = std::function<void()>;

    TIOTask(TIOWorker *context,
            int fd,
            callback_t &callback,
            finish_t &finisher,
            uint32_t events);

    /* Client API */
    void Reconfigure(uint32_t other = 0);

    [[nodiscard]] int Read(char *, size_t);

    int Write(const char *, size_t);

    void Close();

    /* TIOWorker API */
    void Callback(uint32_t events) noexcept;

    void OnDisconnect() noexcept;

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
    finish_t FinishHandler;
    time_point LastAction;
};

#endif //GETADDR_SERVER_IOTASK_H
