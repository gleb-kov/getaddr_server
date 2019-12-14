#include "gai_task.h"

TGetaddrinfoTask::TGetaddrinfoTask()
        : Hints{0, AF_UNSPEC, SOCK_STREAM}, Info(nullptr), Node(nullptr), HaveWork(false), Cancel(false),
          Thread([this] {
              while (true) {
                  std::unique_lock<std::mutex> lg(Mutex);
                  CV.wait(lg, [this] {
                      return Cancel.load() || HaveWork;
                  });

                  if (Cancel) {
                      break;
                  }

                  char *URL = Queries.front().first;
                  size_t URLSize = Queries.front().second;
                  Queries.pop();

                  lg.unlock();
                  std::string result = ProcessNext(URL, URLSize);
                  lg.lock();
                  HaveWork = !Queries.empty();
                  Results.push(result);
              }
          }) {}

void TGetaddrinfoTask::SetTask(char *host, size_t len) {
    std::unique_lock<std::mutex> lg(Mutex);
    if (Queries.size() + Results.size() >= QUERIES_MAX) {
        throw std::runtime_error("TGetaddrinfotask::SetTask() on full queries queue");
    }
    Queries.push({host, len});
    HaveWork = true;
    CV.notify_all();
}

bool TGetaddrinfoTask::HaveFreeSpace() const {
    std::unique_lock<std::mutex> lg(Mutex);
    return Queries.size() + Results.size() < QUERIES_MAX;
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

void TGetaddrinfoTask::Stop() {
    std::unique_lock<std::mutex> lg(Mutex);
    Cancel.store(true);
}

TGetaddrinfoTask::~TGetaddrinfoTask() {
    Cancel.store(true);
    {
        std::unique_lock<std::mutex> lg(Mutex);
        CV.notify_all();
        kill(Thread.native_handle(), SIGINT);
    }
    Thread.join();
}

std::string TGetaddrinfoTask::ProcessNext(char *host, size_t size) {
    std::string tmp = host;
    if (tmp.size() > size) {
        tmp.erase(tmp.begin() + size, tmp.end());
    }
    while (!tmp.empty() && (tmp.back() == '\r' || tmp.back() == '\n')) {
        tmp.pop_back();
    }

    int errorCode = getaddrinfo(tmp.c_str(), nullptr, &Hints, &Info);

    std::string res = "Host: " + tmp + "\n";

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
