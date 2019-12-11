#include "server.h"

namespace {
    volatile sig_atomic_t quitSignalFlag = 0;

    void signalHandler([[maybe_unused]] int signal) {
        quitSignalFlag = 1;
    }
}

TClientTimer::TClientTimer(int timeout) : TimeOut(timeout) {}

void TClientTimer::AddClient(TClient *client) {
    int curtime = 0;
    CachedAction.insert({client, curtime});
    Connections.insert({{curtime, client}, std::unique_ptr<TClient>(client)});
}

void TClientTimer::RefuseClient(TClient *client) {
    // check if it in cachedaction god please noexception
    int cachedLastAction = CachedAction[client];
    Connections.erase({cachedLastAction, client});
}

int TClientTimer::NextCheck() {
    return TimeOut;
}

void TClientTimer::RemoveOld() {
    int curtime = 100;
    FakeConnections.clear();

    for (auto it = Connections.cbegin(); it != Connections.cend();) {
        if (curtime - it->first.first > TimeOut) {
            // FakeConnections.insert({{curtime, it->first.second}, it->second});
            Connections.erase(it++);
        } else {
            break;
        }
    }
}

TIOWorker::TIOWorker(int timeout) : Clients(timeout) {
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

void TIOWorker::Exec() {
    signal(SIGINT, ::signalHandler);
    signal(SIGTERM, ::signalHandler);
    signal(SIGPIPE, SIG_IGN);

    std::array<epoll_event, TIOWORKER_EPOLL_MAX> events{};
    while (true) {
        int count = epoll_wait(efd, events.data(), TIOWORKER_EPOLL_MAX, Clients.NextCheck());

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
                 uint32_t events,
                 int fd,
                 std::function<void(uint32_t, TIOTask *)> callback)
        : Context(context), Events(events), fd(fd), CallbackHandler(std::move(callback)) {
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
    Task = std::make_unique<TIOTask>(&io_context, EPOLLIN, fd, receiver);
}

TClient::TClient(TIOWorker *io_context, int fd) : Context(io_context) {
    std::function<void(uint32_t, TIOTask *)> echo =
            [this, fd](uint32_t events, TIOTask *self) noexcept(true) {
                if ((events & EPOLLERR) || (events & EPOLLRDHUP) || (events & EPOLLHUP)) {
                    Finish();
                    return;
                }
                if (events & EPOLLIN) {
                    int code = recv(fd, &Buffer, DOMAIN_MAX_LENGTH, 0);
                    if (code < 0) {
                        Finish();
                    } else {
                        Queries.push({Buffer, code});
                        epoll_event e{(CLOSE_EVENTS | EPOLLOUT), self};
                        Context->Edit(fd, &e);
                    }
                    return;
                }
                if (events & EPOLLOUT) {
                    int code = send(fd, Queries.front().first.c_str(), Queries.front().second, 0);
                    Queries.pop();

                    if (code < 0) {
                        Finish();
                    } else {
                        epoll_event e{(CLOSE_EVENTS | EPOLLIN), self};
                        Context->Edit(fd, &e);
                    }
                }
            };
    Task = std::make_unique<TIOTask>(Context, (CLOSE_EVENTS | EPOLLIN), fd, echo);
}

void TClient::Finish() {
    Context->RefuseClient(this);
}
