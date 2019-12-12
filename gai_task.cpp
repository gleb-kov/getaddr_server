#include "gai_task.h"

TGetaddrinfoTask::TGetaddrinfoTask()
        : Hints{0, AF_UNSPEC, SOCK_STREAM}
        , Info(nullptr)
        , Node(nullptr)
        , HaveWork(false)
        , Cancel(false)
        , Thread([this] {
            while(true) {
                std::unique_lock<std::mutex> lg(Mutex);
                CV.wait(lg, [this] {
                    return Cancel.load() || HaveWork;
                });

                if (Cancel) {
                    break;
                }

                char * tmp = Queries.front().first;
                size_t tmpsize = Queries.front().second;
                Queries.pop();
                HaveWork = !Queries.empty();

                lg.unlock();
                std::string result = ProcessNext(tmp, tmpsize);
                lg.lock();
                Results.push(result);
            }
        })
{}

void TGetaddrinfoTask::SetTask(char *host, size_t len) {
    std::unique_lock<std::mutex> lg(Mutex);
    Queries.push({host, len});
    HaveWork = true;
}

bool TGetaddrinfoTask::HaveResult() {
    std::unique_lock<std::mutex> lg(Mutex);
    return !Results.empty();
}

std::string TGetaddrinfoTask::GetResult() {
    std::unique_lock<std::mutex> lg(Mutex);
    if (Results.empty()) {
        throw std::runtime_error("Incorrect usage of getaddrtask.");
    }
    std::string tmp = Results.front();
    Results.pop();
    return tmp;
}

void TGetaddrinfoTask::Stop() {
    std::unique_lock<std::mutex> lg(Mutex);
    Cancel.store(true);
}

TGetaddrinfoTask::~TGetaddrinfoTask() {
    Cancel.store(true);
    {
        std::unique_lock<std::mutex> lg(Mutex);
        CV.notify_all();
    }
    Thread.join();
}

std::string TGetaddrinfoTask::ProcessNext(char const * host, size_t size) {
    int errorCode = getaddrinfo(host, nullptr, &Hints, &Info);

    std::string res = "Host";
    res += host;
    res += "\n";

    if (errorCode != 0) {
        res += "Getaddrinfo() failed.";
        // res += "Error: " + errorCode
        res += "\n";
        return res;
    }

    void *ptr = nullptr;
    for (Node = Info; Node; Node = Node->ai_next) {
        if (Node->ai_family == AF_INET) {
            ptr = &(reinterpret_cast<sockaddr_in *>(Node->ai_addr)->sin_addr);
        } else if (Node->ai_family == AF_INET6) {
            ptr = &(reinterpret_cast<sockaddr_in6 *>(Node->ai_addr)->sin6_addr);
        } else {
            continue;
        }
        inet_ntop(Node->ai_family, ptr, addrstr, IP_NODE_MAX_SIZE);
        res += addrstr;
        res += "\n";
    }
    freeaddrinfo(Info);
    return res;
}
