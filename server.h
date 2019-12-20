#ifndef GETADDR_SERVER_SERVER_H
#define GETADDR_SERVER_SERVER_H

#include <cerrno>
#include <cstring>
#include <functional>
#include <memory>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unordered_map>

#include "ioworker.h"
#include "iotask.h"

template <typename T>
class TServer {
public:
    TServer(TIOWorker &io_context, uint32_t address, uint16_t port)
            : Address(address)
            , Port(port)
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            throw std::runtime_error(std::string("TServer() socket() call. ") + std::strerror(errno));
        }

        sockaddr_in sain{};
        sain.sin_family = AF_INET;
        sain.sin_addr.s_addr = Address;
        sain.sin_port = Port;

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
                [&io_context, fd, this](TIOTask *const, uint32_t events) noexcept {
                    if (events != EPOLLIN) {
                        return;
                    }

                    int sfd = accept4(fd, nullptr, nullptr, SOCK_NONBLOCK);
                    if (sfd < 0) {
                        return;
                    }

                    try {
                        std::unique_ptr<T> clientPtr =
                                std::make_unique<T>();
                        T * ptr = clientPtr.get();
                        std::function<void()> finisher = [this, ptr]() noexcept {
                            storage.erase(ptr);
                        };
                        std::unique_ptr<TIOTask> task =
                                std::make_unique<TIOTask>(&io_context,
                                                          sfd,
                                                          clientPtr->callback,
                                                          finisher,
                                                          EPOLLIN);
                        io_context.ConnectClient(task);
                        storage.insert({ptr, std::move(clientPtr)});
                    } catch (...) {}
                };
        Task = std::make_unique<TIOTask>(&io_context, fd, receiver, []{}, EPOLLIN);
    }

    ~TServer() = default;

    TServer(TServer const &) = delete;

    TServer(TServer &&) = delete;

    TServer &operator=(TServer const &) = delete;

    TServer &operator=(TServer &&) = delete;

private:
    const uint32_t Address;
    const uint16_t Port;
    std::unordered_map<T *, std::unique_ptr<T>> storage;
    std::unique_ptr<TIOTask> Task;
};

#endif //GETADDR_SERVER_SERVER_H
