#include "gai_task.h"

TGetaddrinfoTask::TGetaddrinfoTask()
        : Hints{0, AF_UNSPEC, SOCK_STREAM}
        , Info(nullptr)
        , Node(nullptr)
        , Cancel(false)
        /*, Thread([this] {
            while(true) {
                std::unique_lock<std::mutex> lg(Mutex);
                CV.wait(lg, [this] {
                    return Cancel.load();
                });

                if(Cancel) {
                    break;
                }
            }
        })*/
{}

void TGetaddrinfoTask::SetTask(const char *host, size_t len) {}

bool TGetaddrinfoTask::HaveResult() {
    std::unique_lock<std::mutex> lg(Mutex);
    return !Results.empty();
}

std::string TGetaddrinfoTask::GetResult() {
    std::unique_lock<std::mutex> lg(Mutex);
    if (Results.empty()) {
        throw std::runtime_error("Incorrect usage of getaddrtask");
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

void TGetaddrinfoTask::ProcessNext(char const * host) {
    int errorCode = getaddrinfo(host, nullptr, &Hints, &Info);

    std::cerr << "Host: " << host << std::endl;
    if (errorCode != 0) {
        std::cerr << "Getaddrinfo() failed. Error: " << errorCode << std::endl;
        return;
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
        std::cerr << addrstr << std::endl;
    }
    freeaddrinfo(Info);
}
