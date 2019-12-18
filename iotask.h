#ifndef GETADDR_SERVER_IOTASK_H
#define GETADDR_SERVER_IOTASK_H

#include <chrono>
#include <functional>
#include <memory>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ioworker.h"

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

#endif //GETADDR_SERVER_IOTASK_H
