#ifndef GETADDR_SERVER_GETADDRINFO_TASK_H
#define GETADDR_SERVER_GETADDRINFO_TASK_H

#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <queue>

#include <condition_variable>
#include <csignal>
#include <mutex>
#include <thread>

class TGaiTask {
public:
    using result_t = std::string;

    TGaiTask();

    void SetTask(const char *host, size_t len);

    bool HaveFreeSpace() const;

    bool HaveUnprocessed() const;

    bool HaveResult() const;

    [[nodiscard]] result_t GetResult();

    ~TGaiTask();

    TGaiTask(TGaiTask const &) = delete;

    TGaiTask(TGaiTask &&) = delete;

    TGaiTask &operator=(TGaiTask const &) = delete;

    TGaiTask &operator=(TGaiTask &&) = delete;

private:
    [[nodiscard]] result_t ProcessNext(std::string &);

public:
    static const size_t QUERIES_MAX_NUMBER = 1000;
    static const size_t QUERY_MAX_LENGTH = QUERIES_MAX_NUMBER;
    static const size_t DOMAIN_MAX_LENGTH = 255;
    static const size_t IP_NODE_MAX_SIZE = 46;

private:
    const addrinfo Hints;
    char AddrStr[IP_NODE_MAX_SIZE] = {0};

private:
    mutable std::mutex Mutex;
    bool HaveWork;
    bool Cancel;
    std::string QueryPrefix;  // prefix of received domain
    std::queue<std::string> Queries;
    std::queue<result_t> Results;

    std::condition_variable CV;
    std::thread Thread;
};

#endif //GETADDR_SERVER_GETADDRINFO_TASK_H
