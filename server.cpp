#include "server.h"

namespace {
    volatile sig_atomic_t quitSignalFlag = 0;

    void signalHandler([[maybe_unused]] int signal) {
        quitSignalFlag = 1;
    }
}

TClientTimer::TClientTimer(int64_t timeout) : TimeOut(timeout) {}

void TClientTimer::AddClient(TClient *client) {
    time_point curtime = std::chrono::steady_clock::now();
    auto insert1 = CachedAction.insert({client, curtime});
    if (!insert1.second) {
        throw std::runtime_error("Failed insertion into TClientTimer");
    }
    auto insert2 = Connections.insert({{curtime, client}, std::unique_ptr<TClient>(client)});
    if (!insert2.second) {
        CachedAction.erase(client);
        throw std::runtime_error("Failed insertion into TClientTimer");
    }
}

void TClientTimer::RefuseClient(TClient *client) {
    time_point cachedLastAction = CachedAction[client];
    CachedAction.erase(client);
    Connections.erase({cachedLastAction, client});
}

int64_t TClientTimer::NextCheck() {
    if (Connections.empty()) {
        return -1;
    }
    time_point curtime = std::chrono::steady_clock::now();
    int64_t diff = TimeDiff(curtime, Connections.begin()->first.first);
    return (diff > 0 ? diff : -1);
}

void TClientTimer::RemoveOld() {
    time_point curtime = std::chrono::steady_clock::now();
    Fake.clear();

    auto it = Connections.begin();
    for (; it != Connections.end(); it++) {
        if (TimeDiff(curtime, it->first.first) < TimeOut) {
            break;
        }
        time_point realLastTime = it->second->GetLastTime();
        if (TimeDiff(curtime, realLastTime) < TimeOut) {
            Fake.insert({{realLastTime, it->first.second}, std::move(it->second)});
        }
        CachedAction.erase(it->first.second);
    }
    Connections.erase(Connections.begin(), it);
    for (auto & it2 : Fake) {
        CachedAction.insert({it2.first.second, it2.second->GetLastTime()});
        Connections.insert({it2.first, std::move(it2.second)});
    }
}

int64_t TClientTimer::TimeDiff(TClientTimer::time_point const &lhs,
                               TClientTimer::time_point const &rhs) const {
    return std::chrono::duration_cast<std::chrono::seconds>(lhs - rhs).count();
}

TIOWorker::TIOWorker(int64_t sockTimeout) : Clients(sockTimeout) {
    efd = epoll_create1(0);

    if (efd < 0) {
        throw std::runtime_error(std::string("TIOWorker() epoll_create() call. ") + std::strerror(errno));
    }
}

void TIOWorker::ConnectClient(TClient *client) {
    Clients.AddClient(client);
}

void TIOWorker::RefuseClient(TClient *client) {
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
            static_cast<TIOTask *>(it->data.ptr)->Callback(it->events);
        }
        Clients.RemoveOld();
    }
}

TIOTask::TIOTask(TIOWorker *context,
                 int fd,
                 std::function<void(uint32_t, TIOTask *)> &callback,
                 uint32_t events)
        : Context(context)
        , fd(fd)
        , CallbackHandler(std::move(callback)) {
    epoll_event event{events, this};
    Context->Add(fd, &event);
}

void TIOTask::Reconfigure(bool in, bool out) {
    uint32_t events = CLOSE_EVENTS;
    if (in) {
        events |= EPOLLIN;
    }
    if (out) {
        events |= EPOLLOUT;
    }
    epoll_event e{events, this};
    Context->Edit(fd, &e);
}

void TIOTask::Callback(uint32_t events) noexcept {
    CallbackHandler(events, this);
}

TIOTask::~TIOTask() {
    if (Context) {
        epoll_event event{0, this};
        Context->TryRemove(fd, &event);
    }
    shutdown(fd, SHUT_RDWR);
    close(fd);
}

TServer::TServer(TIOWorker &io_context, uint32_t address, uint16_t port)
        : Address(address), Port(port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        throw std::runtime_error(std::string("TServer() socket() call. ") + std::strerror(errno));
    }

    sockaddr_in sain;
    sain.sin_family = AF_INET;
    sain.sin_addr.s_addr = address;
    sain.sin_port = port;

    /* some magic to fix bind() EADDRINUSE on free port */
    int optVal = 1;
    int sockOptCode = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof optVal);
    if (sockOptCode < 0) {
        throw std::runtime_error(std::string("TServer() setsockopt() call. ") + std::strerror(errno));
    }

    int bindCode = bind(fd, reinterpret_cast<sockaddr const *>(&sain), sizeof sain);
    if (bindCode < 0) {
        throw std::runtime_error(std::string("TServer() bind() call. ") + std::strerror(errno));
    }

    int listenCode = listen(fd, SOMAXCONN);
    if (listenCode < 0) {
        throw std::runtime_error(std::string("TServer() listen() call. ") + std::strerror(errno));
    }

    std::function<void(uint32_t, TIOTask *)> receiver =
            [&io_context, fd](uint32_t events, TIOTask *self) noexcept(true) {
                if (events != EPOLLIN) {
                    return;
                }

                int sfd = accept4(fd, nullptr, nullptr, SOCK_NONBLOCK);
                if (sfd < 0) {
                    return;
                }

                TClient *clientPtr = nullptr;
                try {
                    clientPtr = new TClient(&io_context, sfd);
                    io_context.ConnectClient(clientPtr);
                } catch (...) {
                    delete clientPtr;
                }
            };
    Task = std::make_unique<TIOTask>(&io_context, fd, receiver, EPOLLIN);
}

TClient::TClient(TIOWorker *io_context, int fd) : Context(io_context) {
    std::function<void(uint32_t, TIOTask *)> echo =
            [this, fd](uint32_t events, TIOTask *self) noexcept(true) {
                if ((events & EPOLLERR) || (events & EPOLLRDHUP) || (events & EPOLLHUP)) {
                    Finish();
                    return;
                }

                if ((events & EPOLLIN) && QueryProcesser.HaveFreeSpace()) {
                    int code = recv(fd, &Buffer, DOMAIN_MAX_LENGTH, 0);
                    if (code < 0) {
                        Finish();
                    } else {
                        QueryProcesser.SetTask(Buffer, code);
                    }
                    LastAction = std::chrono::steady_clock::now();
                } else if ((events & EPOLLOUT) && QueryProcesser.HaveResult()) {
                    std::string tmp = QueryProcesser.GetResult();
                    int code = send(fd, tmp.c_str(), tmp.size(), 0);

                    if (code < 0) {
                        Finish();
                    }
                    LastAction = std::chrono::steady_clock::now();
                }
                Configure();
            };
    Task = std::make_unique<TIOTask>(Context, fd, echo, (TIOTask::CLOSE_EVENTS | EPOLLIN | EPOLLOUT));
}

std::chrono::steady_clock::time_point TClient::GetLastTime() const {
    return LastAction;
}

void TClient::Configure() {
    bool in = QueryProcesser.HaveFreeSpace();
    bool out = QueryProcesser.HaveUnprocessed();
    Task->Reconfigure(in, out);
}

void TClient::Finish() {
    Context->RefuseClient(this);
}
