#ifndef GETADDR_SERVER_GETADDRINFO_TASK_H
#define GETADDR_SERVER_GETADDRINFO_TASK_H

#include <arpa/inet.h>
#include <iostream>
#include <netdb.h>
#include <string>
#include <queue>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

class TGetaddrinfoTask {
public:
    TGetaddrinfoTask();

    void SetTask(const char *);

    bool HaveResult();

    const char *GetResult();

    void Stop();

    ~TGetaddrinfoTask();

    TGetaddrinfoTask(TGetaddrinfoTask const &) = delete;

    TGetaddrinfoTask(TGetaddrinfoTask &&) = delete;

    TGetaddrinfoTask &operator=(TGetaddrinfoTask const &) = delete;

    TGetaddrinfoTask &operator=(TGetaddrinfoTask &&) = delete;

/*private:
    static const size_t IP_NODE_MAX_SIZE = 46;

    addrinfo Hints;
    addrinfo *Info;
    addrinfo *Node;
    char addrstr[IP_NODE_MAX_SIZE];*/

private:
    mutable std::mutex Mutex;

    std::queue<char const *> Queries;
    std::queue<char const *> Results;

    std::atomic<bool> Cancel;
    std::condition_variable cv;
    std::thread Thread;
};

#endif //GETADDR_SERVER_GETADDRINFO_TASK_H
