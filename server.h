#ifndef GETADDR_SERVER_SERVER_H
#define GETADDR_SERVER_SERVER_H

#include <memory>
#include <unordered_map>
#include <netinet/in.h>
#include <sys/socket.h>

#include "iojob.h"

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

    // std::unique_ptr<TIOTask> Task;

    std::unordered_map<TIOTask *, std::unique_ptr<TIOTask>> Connections;
};


#endif //GETADDR_SERVER_SERVER_H
