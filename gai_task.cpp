#include "gai_task.h"

TGetaddrinfoTask::TGetaddrinfoTask()
        : Hints{0, AF_UNSPEC, SOCK_STREAM}
        , Info(nullptr)
        , Node(nullptr)
        , HaveWork(false)
        , Cancel(false),
          Thread([this] {
              while (true) {
                  std::unique_lock<std::mutex> lg(Mutex);
                  CV.wait(lg, [this] {
                      return Cancel || HaveWork;
                  });

                  if (Cancel) {
                      break;
                  }

                  std::string URL = Queries.front();
                  Queries.pop();
                  lg.unlock();
                  std::string result = ProcessNext(URL);
                  lg.lock();
                  HaveWork = !Queries.empty();
                  Results.push(result);
              }
          })
{}

void TGetaddrinfoTask::SetTask(const char *host, size_t len) {
    if (host == nullptr) return;
    std::unique_lock<std::mutex> lg(Mutex);
    if (Queries.size() + Results.size() >= QUERIES_MAX_NUMBER) {
        throw std::runtime_error("TGetaddrinfotask::SetTask() on full queries queue");
    }
    if (QueryPrefix.size() > DOMAIN_MAX_LENGTH) {
        throw std::out_of_range("Too long domain.");
    }
    for (size_t i = 0; i < len; i++) {
        if (host[i] == '\0' || host[i] == '\r' || host[i] == '\n') {
            if (!QueryPrefix.empty()) {
                Queries.push(QueryPrefix);
            }
            QueryPrefix.clear();
            continue;
        }
        QueryPrefix += host[i];
    }
    HaveWork = !Queries.empty();
    CV.notify_all();
}

bool TGetaddrinfoTask::HaveFreeSpace() const {
    std::unique_lock<std::mutex> lg(Mutex);
    return Queries.size() + Results.size() < QUERIES_MAX_NUMBER;
}

bool TGetaddrinfoTask::HaveUnprocessed() const {
    std::unique_lock<std::mutex> lg(Mutex);
    return HaveWork;
}

bool TGetaddrinfoTask::HaveResult() const {
    std::unique_lock<std::mutex> lg(Mutex);
    return !Results.empty();
}

std::string TGetaddrinfoTask::GetResult() {
    std::unique_lock<std::mutex> lg(Mutex);
    if (Results.empty()) {
        throw std::runtime_error("TGetaddrinfotask::GetResult() on empty results queue.");
    }
    std::string tmp = Results.front();
    Results.pop();
    return tmp;
}

TGetaddrinfoTask::~TGetaddrinfoTask() {
    {
        std::unique_lock<std::mutex> lg(Mutex);
        Cancel = true;
        CV.notify_all();
        kill(Thread.native_handle(), SIGINT);
    }
    Thread.join();
}

std::string TGetaddrinfoTask::ProcessNext(std::string &host) {
    int errorCode = getaddrinfo(host.c_str(), nullptr, &Hints, &Info);
    std::string res = "Host: " + host + "\n";

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
        inet_ntop(Node->ai_family, ptr, AddrStr, IP_NODE_MAX_SIZE);
        res += AddrStr;
        res += "\n";
    }
    freeaddrinfo(Info);
    return res;
}
