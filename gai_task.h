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

    void SetTask(char *, size_t);

    size_t GetFreeSpace() const;

    bool AllProcessed() const;

    bool HaveResult() const;

    ResultType GetResult();

    void Stop();

    ~TGetaddrinfoTask();

    TGetaddrinfoTask(TGetaddrinfoTask const &) = delete;

    TGetaddrinfoTask(TGetaddrinfoTask &&) = delete;

    TGetaddrinfoTask &operator=(TGetaddrinfoTask const &) = delete;

    TGetaddrinfoTask &operator=(TGetaddrinfoTask &&) = delete;

private:
    ResultType ProcessNext(char *, [[maybe_unused]] size_t);

private:
    static const size_t IP_NODE_MAX_SIZE = 46;
    static const size_t QUERIES_MAX = 1000;

    addrinfo Hints;
    addrinfo *Info;
    addrinfo *Node;
    char addrstr[IP_NODE_MAX_SIZE];

private:
    mutable std::mutex Mutex;
    bool HaveWork;
    std::queue<std::pair<char *, size_t>> Queries;
    std::queue<ResultType> Results;

    std::atomic<bool> Cancel;
    std::condition_variable CV;
    std::thread Thread;
};

#endif //GETADDR_SERVER_GETADDRINFO_TASK_H
