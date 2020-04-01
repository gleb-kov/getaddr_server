# Project description
I/O multiplexing library and multithreaded getaddrinfo-server, based on this library.

# How to start:
Starting server:
strace ./getaddr_server

Connect to server:
telnet 127.0.0.1 58239

# Parameters
client timeout:
sockTimeout in TIOWorker constructor, default: 600 seconds.

epoll timer:
epollTimeout in TIOWorker.Exec(), default: no timer.

host and port:
address and port in TServer constructor.
Make sure you called htonl() and htons() before.

main server task:
template parameter in TServer.
