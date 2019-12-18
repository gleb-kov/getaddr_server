#include "server.h"

namespace {
    volatile sig_atomic_t quitSignalFlag = 0;

    void signalHandler([[maybe_unused]] int signal) {
        quitSignalFlag = 1;
    }
}

TIOWorker::TClientTimer::TClientTimer(int64_t timeout) : TimeOut(timeout) {}

void TIOWorker::TClientTimer::AddClient(std::unique_ptr<TClient> &client) {
    time_point curTime = std::chrono::steady_clock::now();
    TClient * const con = client.get();
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

void TIOWorker::TClientTimer::RefuseClient(TClient *client) {
    time_point cachedLastAction = CachedAction[client];
    CachedAction.erase(client);
    Connections.erase({cachedLastAction, client});
}

int64_t TIOWorker::TClientTimer::NextCheck() {
    if (Connections.empty()) {
        return -1;
    }
    time_point curtime = std::chrono::steady_clock::now();
    int64_t diff = TimeDiff(curtime, Connections.begin()->first.first);
    return (diff > 0 ? diff : -1);
}

void TIOWorker::TClientTimer::RemoveOld() {
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

int64_t TIOWorker::TClientTimer::TimeDiff(time_point const &lhs,
                                          time_point const &rhs) const {
    return std::chrono::duration_cast<std::chrono::seconds>(lhs - rhs).count();
}

TIOWorker::TIOWorker(int64_t sockTimeout) : Clients(sockTimeout) {
    efd = epoll_create1(0);

    if (efd < 0) {
        throw std::runtime_error(std::string("TIOWorker() epoll_create() call. ") + std::strerror(errno));
    }
}

void TIOWorker::ConnectClient(std::unique_ptr<TClient> &client) {
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

TIOTask::TIOTask(TIOWorker *const context,
                 int fd,
                 callback_t &callback,
                 std::function<void()> &finisher,
                 uint32_t events)
        : Context(context)
        , fd(fd)
        , CallbackHandler(std::move(callback))
        , FinishHandler(std::move(finisher))
{
    epoll_event event{(CLOSE_EVENTS | events), {this}};
    Context->Add(fd, &event);
}

void TIOTask::SetTask(callback_t &callback) {
    CallbackHandler = std::move(callback);
}

void TIOTask::Reconfigure(uint32_t other) {
    epoll_event e{(CLOSE_EVENTS | other), {this}};
    Context->Edit(fd, &e);
}

int TIOTask::Read(char *buffer, size_t size) {
    int code = recv(fd, buffer, size, 0);
    /*if (code < 0) {//
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
    if (IsClosingEvent(events) || Destroy) {
        FinishHandler();
    } else {
        CallbackHandler(this, events);
    }
}

TIOTask::time_point TIOTask::GetLastTime() const {
    return LastAction;
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

TServer::TServer(TIOWorker &io_context, uint32_t address, uint16_t port)
        : Address(address)
        , Port(port)
{
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

    TIOTask::callback_t receiver =
            [&io_context, fd](TIOTask *const, uint32_t events) noexcept {
                if (events != EPOLLIN) {
                    return;
                }

                int sfd = accept4(fd, nullptr, nullptr, SOCK_NONBLOCK);
                if (sfd < 0) {
                    return;
                }

                try {
                    std::unique_ptr<TClient> clientPtr =
                            std::make_unique<TClient>(&io_context, sfd, EPOLLIN);
                    io_context.ConnectClient(clientPtr);
                } catch (...) {}
            };
    std::function<void()> finisher = [] {};
    Task = std::make_unique<TIOTask>(&io_context, fd, receiver, finisher, EPOLLIN);
}

TClient::TClient(TIOWorker *const io_context, int fd, uint32_t startEvents) {
    std::function<void()> finish = [this, io_context] { io_context->RefuseClient(this); };

    TIOTask::callback_t callback =
            [this](TIOTask *const self, uint32_t events) noexcept {
                if ((events & EPOLLOUT) && QueryProcessor.HaveResult()) {
                    std::string tmp = QueryProcessor.GetResult();
                    self->Write(tmp.c_str(), tmp.size());
                } else if ((events & EPOLLIN) && QueryProcessor.HaveFreeSpace()) {
                    int code = self->Read(Buffer, TGetaddrinfoTask::QUERY_MAX_LENGTH);
                    if (code > 0) {
                        try {
                            QueryProcessor.SetTask(Buffer, code);
                        } catch(...) {
                            self->Close();
                        }
                    }
                }
                uint32_t actions = 0;
                if (QueryProcessor.HaveFreeSpace()) {
                    actions |= EPOLLIN;
                }
                if (QueryProcessor.HaveUnprocessed() || QueryProcessor.HaveResult()) {
                    actions |= EPOLLOUT;
                }
                self->Reconfigure(actions);
            };
    Task = std::make_unique<TIOTask>(io_context, fd, callback, finish, startEvents);
}

TClient::time_point TClient::GetLastTime() const {
    return Task->GetLastTime();
}
