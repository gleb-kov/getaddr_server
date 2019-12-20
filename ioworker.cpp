#include "ioworker.h"
#include "iotask.h"

namespace {
    volatile sig_atomic_t quitSignalFlag = 0;

    void signalHandler([[maybe_unused]] int signal) {
        quitSignalFlag = 1;
    }
}

TIOWorker::TTimer::TTimer(int64_t timeout) : TimeOut(timeout) {}

void TIOWorker::TTimer::AddClient(std::unique_ptr<TIOTask> &client) {
    time_point curTime = std::chrono::steady_clock::now();
    TIOTask * const con = client.get();
    auto insert1 = CachedAction.insert({con, curTime});
    if (!insert1.second) {
        throw std::runtime_error("Failed insertion into TClientTimer");
    }
    auto insert2 = Connections.insert({{curTime, con}, std::move(client)});
    if (!insert2.second) {
        CachedAction.erase(con);
        throw std::runtime_error("Failed insertion into TClientTimer");
    }
}

void TIOWorker::TTimer::RefuseClient(TIOTask *client) {
    time_point cachedLastAction = CachedAction[client];
    CachedAction.erase(client);
    Connections.erase({cachedLastAction, client});
}

int64_t TIOWorker::TTimer::NextCheck() {
    if (Connections.empty()) {
        return -1;
    }
    time_point curtime = std::chrono::steady_clock::now();
    int64_t diff = TimeDiff(curtime, Connections.begin()->first.first);
    return (diff > 0 ? diff : -1);
}

void TIOWorker::TTimer::RemoveOld() {
    time_point curTime = std::chrono::steady_clock::now();
    Fake.clear();

    auto it = Connections.begin();
    for (; it != Connections.end(); it++) {
        if (TimeDiff(curTime, it->first.first) < TimeOut) {
            break;
        }
        time_point realLastTime = it->second->GetLastTime();
        if (TimeDiff(curTime, realLastTime) < TimeOut) {
            Fake.insert({{realLastTime, it->first.second}, std::move(it->second)});
        }
        CachedAction.erase(it->first.second);
    }
    Connections.erase(Connections.begin(), it);
    for (auto &it2 : Fake) {
        CachedAction.insert({it2.first.second, it2.second->GetLastTime()});
        Connections.insert({it2.first, std::move(it2.second)});
    }
}

int64_t TIOWorker::TTimer::TimeDiff(time_point const &lhs,
                                    time_point const &rhs) const {
    return std::chrono::duration_cast<std::chrono::seconds>(lhs - rhs).count();
}

TIOWorker::TIOWorker(int64_t sockTimeout) : Clients(sockTimeout) {
    efd = epoll_create1(0);

    if (efd < 0) {
        throw std::runtime_error(std::string("TIOWorker() epoll_create() call. ") + std::strerror(errno));
    }
}

void TIOWorker::ConnectClient(std::unique_ptr<TIOTask> &client) {
    Clients.AddClient(client);
}

void TIOWorker::RefuseClient(TIOTask *client) {
    Clients.RefuseClient(client);
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

void TIOWorker::Exec(int64_t epollTimeout) {
    signal(SIGINT, ::signalHandler);
    signal(SIGTERM, ::signalHandler);
    signal(SIGPIPE, SIG_IGN);

    std::array<epoll_event, TIOWORKER_EPOLL_MAX> events{};
    while (true) {
        int count = epoll_wait(efd, events.data(), TIOWORKER_EPOLL_MAX, epollTimeout);

        if (::quitSignalFlag == 1) {
            return;
        }

        if (count < 0) {
            throw std::runtime_error(std::string("TIOWorker::Exec() epoll_wait() call. ") + std::strerror(errno));
        }

        for (auto it = events.begin(); it != events.begin() + count; it++) {
            auto curTask = static_cast<TIOTask *>(it->data.ptr);
            if (TIOTask::IsClosingEvent(it->events) || curTask->Destroying()) {
                curTask->OnDisconnect();
                RefuseClient(curTask);
            } else {
                curTask->Callback(it->events);
            }
        }
        Clients.RemoveOld();
    }
}