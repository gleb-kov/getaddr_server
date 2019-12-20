#include "iotask.h"

TIOTask::TIOTask(TIOWorker *const context,
                 int fd,
                 callback_t &callback,
                 finish_t &finisher,
                 uint32_t events)
        : Context(context)
        , fd(fd)
        , CallbackHandler(std::move(callback))
        , FinishHandler(std::move(finisher))
{
    epoll_event event{(CLOSE_EVENTS | events), {this}};
    Context->Add(fd, &event);
}

TIOTask::TIOTask(TIOWorker *const context,
                 int fd,
                 callback_t &callback,
                 finish_t &&finisher,
                 uint32_t events)
        : Context(context)
        , fd(fd)
        , CallbackHandler(std::move(callback))
        , FinishHandler(std::move(finisher))
{
    epoll_event event{(CLOSE_EVENTS | events), {this}};
    Context->Add(fd, &event);
}

void TIOTask::Reconfigure(uint32_t other) {
    epoll_event e{(CLOSE_EVENTS | other), {this}};
    Context->Edit(fd, &e);
}

int TIOTask::Read(char *buffer, size_t size) {
    int code = recv(fd, buffer, size, 0);
    /*if (code < 0) {
        Close();
    }*/
    UpdateTime();
    return code;
}

int TIOTask::Write(const char *buffer, size_t size) {
    int code = send(fd, buffer, size, 0);
    /*if (code < 0) {
        Close();
    }*/
    UpdateTime();
    return code;
}

void TIOTask::Close() {
    Destroy = true;
}

void TIOTask::Callback(uint32_t events) noexcept {
    CallbackHandler(this, events);
}

void TIOTask::OnDisconnect() noexcept {
    FinishHandler();
}

TIOTask::time_point TIOTask::GetLastTime() const {
    return LastAction;
}

bool TIOTask::Destroying() const {
    return Destroy;
}

void TIOTask::UpdateTime() {
    LastAction = std::chrono::steady_clock::now();
}

TIOTask::~TIOTask() {
    if (Context) {
        epoll_event event{0, {this}};
        Context->TryRemove(fd, &event);
    }
    shutdown(fd, SHUT_RDWR);
    close(fd);
}

bool TIOTask::IsClosingEvent(uint32_t events) {
    return ((events & EPOLLERR) || (events & EPOLLRDHUP) || (events & EPOLLHUP));
}
