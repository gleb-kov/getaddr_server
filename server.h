#ifndef GETADDR_SERVER_SERVER_H
#define GETADDR_SERVER_SERVER_H

#include <cerrno>
#include <cstring>
#include <functional>
#include <memory>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unordered_map>

#include "ioworker.h"
#include "iotask.h"
#include "gai_task.h"

class TClient;

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
    std::unordered_map<TClient *, std::unique_ptr<TClient>> storage;
    std::unique_ptr<TIOTask> Task;
};

class TClient {
public:
    TClient(TIOWorker *io_context, int fd);

    ~TClient() = default;

    TClient(TClient const &) = delete;

    TClient(TClient &&) = delete;

    TClient &operator=(TClient const &) = delete;

    TClient &operator=(TClient &&) = delete;

public:
    TIOTask::callback_t callback;
    std::unique_ptr<TIOTask> Task;

private:
    char Buffer[TGetaddrinfoTask::QUERY_MAX_LENGTH] = {0};
    std::string ResultSuffix;
    TGetaddrinfoTask QueryProcessor;
};

#endif //GETADDR_SERVER_SERVER_H
