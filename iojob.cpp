#include "iojob.h"

namespace {
    volatile sig_atomic_t quitSignalFlag = 0;

    void signalHandler([[maybe_unused]] int signal) {
        quitSignalFlag = 1;
    }
}

TIOWorker::TIOWorker() {
    efd = epoll_create1(0);

    if (efd < 0) {
        throw std::runtime_error(std::string("TIOWorker() epoll_create() call. ") + std::strerror(errno));
    }
}

void TIOWorker::Add(int fd, epoll_event *task) {
    int ctlCode = epoll_ctl(efd, EPOLL_CTL_ADD, fd, task);

    if (ctlCode < 0) {
        throw std::runtime_error(std::string("TIOWorker()::Add() epoll_ctl() call. ") + std::strerror(errno));
    }
}

void TIOWorker::Edit(int fd, epoll_event *task) {
    int ctlCode = epoll_ctl(efd, EPOLL_CTL_MOD, fd, task);

    if (ctlCode < 0) {
        throw std::runtime_error(std::string("TIOWorker()::Edit() epoll_ctl() call. ") + std::strerror(errno));
    }
}

void TIOWorker::Remove(int fd, epoll_event *task) {
    int ctlCode = epoll_ctl(efd, EPOLL_CTL_DEL, fd, task);

    if (ctlCode < 0) {
        throw std::runtime_error(std::string("TIOWorker()::Remove() epoll_ctl() call.") + std::strerror(errno));
    }
}

int TIOWorker::TryRemove(int fd, epoll_event *task) noexcept {
    int ctlCode = epoll_ctl(efd, EPOLL_CTL_DEL, fd, task);
    return ctlCode;
}

void TIOWorker::Exec(int timeout) {
    signal(SIGINT, ::signalHandler);
    signal(SIGTERM, ::signalHandler);
    signal(SIGPIPE, SIG_IGN);

    std::array<epoll_event, TIOWORKER_EPOLL_MAX> events{};
    while (true) {
        int count = epoll_wait(efd, events.data(), TIOWORKER_EPOLL_MAX, timeout);

        if (::quitSignalFlag == 1) {
            return;
        }

        if (count < 0) {
            throw std::runtime_error(std::string("TIOWorker::Exec() epoll_wait() call. ") + std::strerror(errno));
        }

        for (auto it = events.begin(); it != events.begin() + count; it++) {
            static_cast<TIOTask *>(it->data.ptr)->Callback(it->events);
        }
    }
}

TIOTask::TIOTask(TIOWorker *context,
                 uint32_t events,
                 int fd,
                 std::function<void(uint32_t, TIOTask *)> callback)
        : Context(context)
        , Events(events)
        , fd(fd)
        , CallbackHandler(std::move(callback))
{
    epoll_event event{Events, this};
    Context->Add(fd, &event);
}

void TIOTask::Callback(uint32_t events) noexcept {
    CallbackHandler(events, this);
}

TIOTask::~TIOTask() {
    if (Context) {
        epoll_event event{Events, this};
        Context->TryRemove(fd, &event);
    }
    shutdown(fd, SHUT_RDWR);
    close(fd);
}
