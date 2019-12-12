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
    using ResultType = std::string;

public:
    TGetaddrinfoTask();

    void SetTask(const char *, size_t);

    bool HaveResult();

    ResultType GetResult();

    void Stop();

    ~TGetaddrinfoTask();

    TGetaddrinfoTask(TGetaddrinfoTask const &) = delete;

    TGetaddrinfoTask(TGetaddrinfoTask &&) = delete;

    TGetaddrinfoTask &operator=(TGetaddrinfoTask const &) = delete;

    TGetaddrinfoTask &operator=(TGetaddrinfoTask &&) = delete;

private:
    void ProcessNext(char const *);

private:
    static const size_t IP_NODE_MAX_SIZE = 46;

    addrinfo Hints;
    addrinfo *Info;
    addrinfo *Node;
    char addrstr[IP_NODE_MAX_SIZE];

private:
    mutable std::mutex Mutex;
    std::queue<char const *> Queries;
    std::queue<ResultType> Results;

    std::atomic<bool> Cancel;
    std::condition_variable CV;
    std::thread Thread;
};

#endif //GETADDR_SERVER_GETADDRINFO_TASK_H
