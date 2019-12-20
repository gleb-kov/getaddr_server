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
#include "gai_client.h"

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
    std::unordered_map<TGaiClient *, std::unique_ptr<TGaiClient>> storage;
    std::unique_ptr<TIOTask> Task;
};

#endif //GETADDR_SERVER_SERVER_H
