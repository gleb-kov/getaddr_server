#ifndef GETADDR_SERVER_SERVER_H
#define GETADDR_SERVER_SERVER_H

#include <cerrno>
#include <cstring>
#include <functional>
#include <memory>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ioworker.h"
#include "iotask.h"
#include "gai_task.h"

class TServer {
public:
    TServer(TIOWorker &io_context, uint32_t address, uint16_t port);

    ~TServer() = default;

    TServer(TServer const &) = delete;

    TServer(TServer &&) = delete;

    TServer &operator=(TServer const &) = delete;

    TServer &operator=(TServer &&) = delete;

private:
    const uint32_t Address;
    const uint16_t Port;
    std::unique_ptr<TIOTask> Task;
};

class TClient {
public:
    using time_point = TIOWorker::time_point;

    TClient(TIOWorker *io_context, int fd, uint32_t startEvents);

    time_point GetLastTime() const;

    ~TClient() = default;

    TClient(TClient const &) = delete;

    TClient(TClient &&) = delete;

    TClient &operator=(TClient const &) = delete;

    TClient &operator=(TClient &&) = delete;

private:
    char Buffer[TGetaddrinfoTask::QUERY_MAX_LENGTH] = {0};
    TGetaddrinfoTask QueryProcessor;
    std::unique_ptr<TIOTask> Task;
};

#endif //GETADDR_SERVER_SERVER_H
