#include "iojob.h"

TIOWorker::TIOWorker() {
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
    std::array<epoll_event, TIOWORKER_EPOLL_MAX> events{};
    while (true) {
        int count = epoll_wait(efd, events.data(), TIOWORKER_EPOLL_MAX, timeout);

        if (count < 0) {
            throw std::runtime_error("ERROR: epoll_wait() returned some negative number");
        }

        for (auto it = events.begin(); it != events.begin() + count; it++) {
            // try catch ?
            static_cast<TIOTask *>(it->data.ptr)->Callback(it->events);
        }
    }
}

TIOTask::TIOTask(TIOWorker *context, uint32_t events, int fd, std::function<void(uint32_t)> callback)
        : Context(context), Events(events), fd(fd), Callback_handler(std::move(callback)) {
    epoll_event event;
    event.events = events;
    // event.data.fd = fd;
    event.data.ptr = this;
    Context->Add(fd, &event);
}

void TIOTask::Callback(uint32_t events) {
    //simple, only for accepting sockets
    Callback_handler(events);
}

TIOTask::~TIOTask() {
    // close(fd);
}
