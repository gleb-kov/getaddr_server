#include "server.h"

TServer::TServer(TIOWorker &io_context, uint32_t address, uint16_t port)
    : Address(address)
    , Port(port)
{
    fd = socket(AF_INET, SOCK_STREAM, 0);
    // if (fd < 0)

    sockaddr_in sain;
    sain.sin_family = AF_INET;
    sain.sin_addr.s_addr = htonl(address);
    sain.sin_port = htons(port);

    int bind_code = bind(fd, reinterpret_cast<sockaddr const *>(&sain), sizeof sain);
    // if (bind_code < 0)

    int listen_code = listen(fd, SOMAXCONN); // bool?
    // if (listen_code < 0)
}
