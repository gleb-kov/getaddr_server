#ifndef GETADDR_SERVER_SERVER_H
#define GETADDR_SERVER_SERVER_H

#include <memory>
// #include <unordered_map>
#include <netinet/in.h>
#include <sys/socket.h>

#include "iojob.h"

// class TClient {};

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

    int fd;

    // std::unordered_map<TClient *, std::unique_ptr<TClient>> Connections;
};


#endif //GETADDR_SERVER_SERVER_H
