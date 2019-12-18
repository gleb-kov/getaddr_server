#ifndef GETADDR_SERVER_GETADDRINFO_TASK_H
#define GETADDR_SERVER_GETADDRINFO_TASK_H

#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <queue>

#include <atomic>
#include <condition_variable>
#include <csignal>
#include <mutex>
#include <thread>

class TGetaddrinfoTask {
    using result_t = std::string;

public:
    TGetaddrinfoTask();

    void SetTask(const char *, size_t);

    bool HaveFreeSpace() const;

    bool HaveUnprocessed() const;

    bool HaveResult() const;

    result_t GetResult();

    ~TGetaddrinfoTask();

    TGetaddrinfoTask(TGetaddrinfoTask const &) = delete;

    TGetaddrinfoTask(TGetaddrinfoTask &&) = delete;

    TGetaddrinfoTask &operator=(TGetaddrinfoTask const &) = delete;

    TGetaddrinfoTask &operator=(TGetaddrinfoTask &&) = delete;

private:
    result_t ProcessNext(std::string &);

public:
    static const size_t QUERIES_MAX_NUMBER = 1000;
    static const size_t QUERY_MAX_LENGTH = QUERIES_MAX_NUMBER;
    static const size_t DOMAIN_MAX_LENGTH = 255;
    static const size_t IP_NODE_MAX_SIZE = 46;

private:
    addrinfo Hints;
    addrinfo *Info;
    addrinfo *Node;
    char AddrStr[IP_NODE_MAX_SIZE] = {0};

private:
    mutable std::mutex Mutex;
    bool HaveWork;
    bool Cancel;
    std::string QueryPrefix;  // first part of received domain
    // std::string ResultSuffix; // last part of sending domain
    std::queue<std::string> Queries;
    std::queue<result_t> Results;

    std::condition_variable CV;
    std::thread Thread;
};

#endif //GETADDR_SERVER_GETADDRINFO_TASK_H
