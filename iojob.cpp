#include "iojob.h"

namespace {
    volatile sig_atomic_t quitSignalFlag = 0;

    void signalHandler([[maybe_unused]] int signal) {
        quitSignalFlag = 1;
    }
}

TIOWorker::TIOWorker() noexcept(false) {
    efd = epoll_create1(0);

    if (efd < 0) {
        throw std::runtime_error("ERROR: Epoll creation failed");
    }
}

void TIOWorker::Add(int fd, epoll_event *task) {
    int ctl_code = epoll_ctl(efd, EPOLL_CTL_ADD, fd, task);

    if (ctl_code < 0) {
        throw std::runtime_error("ERROR: add event to epoll");
    }
}

void TIOWorker::Edit(int fd, epoll_event *task) {
    int ctl_code = epoll_ctl(efd, EPOLL_CTL_MOD, fd, task);

    if (ctl_code < 0) {
        throw std::runtime_error("ERROR: edit event on epoll");
    }
}

void TIOWorker::Remove(int fd, epoll_event *task) {
    int ctl_code = epoll_ctl(efd, EPOLL_CTL_DEL, fd, task);

    if (ctl_code < 0) {
        throw std::runtime_error("ERROR: Remove event from epoll");
    }
}

void TIOWorker::Exec(int timeout) {
    signal(SIGINT, ::signalHandler);
    signal(SIGTERM, ::signalHandler);
    signal(SIGPIPE, SIG_IGN);

    std::array<epoll_event, TIOWORKER_EPOLL_MAX> events{};
    while (true) {
        int count = epoll_wait(efd, events.data(), TIOWORKER_EPOLL_MAX, timeout);

        if (::quitSignalFlag == 1)
            return;

        if (count < 0) {
            throw std::runtime_error("ERROR: epoll_wait() returned some negative number");
        }

        for (auto it = events.begin(); it != events.begin() + count; it++) {
            static_cast<TIOTask *>(it->data.ptr)->Callback(it->events); // exception ?
        }
    }
}

TIOTask::TIOTask(TIOWorker *context, uint32_t events, int fd, std::function<void(uint32_t)> callback)
        : Context(context), Events(events), fd(fd), CallbackHandler(std::move(callback)) {
    epoll_event event{Events, this};
    Context->Add(fd, &event); // exception ?
}

void TIOTask::Callback(uint32_t events) {
    CallbackHandler(events); // exception ?
}

TIOTask::~TIOTask() {
    if (Context) {
        epoll_event event{Events, this};
        Context->Remove(fd, &event); // exception ?
    }
    shutdown(fd, SHUT_RDWR);
    close(fd);
}
