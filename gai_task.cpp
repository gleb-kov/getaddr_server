#include "gai_task.h"

TGaiTask::TGaiTask()
        : Hints{0, AF_UNSPEC, SOCK_STREAM}
        , HaveWork(false)
        , Cancel(false)
        , Thread([this] {
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
                  result_t result = ProcessNext(URL);
                  lg.lock();
                  HaveWork = !Queries.empty();
                  Results.push(result);
              }
          })
{}

void TGaiTask::SetTask(const char *host, size_t len) {
    if (host == nullptr) return;
    std::unique_lock<std::mutex> lg(Mutex);
    if (Queries.size() + Results.size() >= QUERIES_MAX_NUMBER) {
        throw std::runtime_error("TGaiTask::SetTask() on full queries queue");
    }
    if (QueryPrefix.size() > DOMAIN_MAX_LENGTH) {
        throw std::out_of_range("Too long domain.");
    }
    size_t last = 0;
    for (size_t i = 0; i < len; i++) {
        if (host[i] == '\0' || host[i] == '\r' || host[i] == '\n') {
            QueryPrefix.append(host + last, host + i);
            last = i + 1;
            if (!QueryPrefix.empty()) {
                Queries.push(QueryPrefix);
            }
            QueryPrefix.clear();
        }
    }
    QueryPrefix.append(host + last, host + len);
    HaveWork = !Queries.empty();
    CV.notify_all();
}

bool TGaiTask::HaveFreeSpace() const {
    std::unique_lock<std::mutex> lg(Mutex);
    return Queries.size() + Results.size() < QUERIES_MAX_NUMBER;
}

bool TGaiTask::HaveUnprocessed() const {
    std::unique_lock<std::mutex> lg(Mutex);
    return HaveWork;
}

bool TGaiTask::HaveResult() const {
    std::unique_lock<std::mutex> lg(Mutex);
    return !Results.empty();
}

TGaiTask::result_t TGaiTask::GetResult() {
    std::unique_lock<std::mutex> lg(Mutex);
    if (Results.empty()) {
        throw std::runtime_error("TGaiTask::GetResult() on empty results queue.");
    }
    result_t tmp = Results.front();
    Results.pop();
    return tmp;
}

TGaiTask::~TGaiTask() {
    {
        std::unique_lock<std::mutex> lg(Mutex);
        Cancel = true;
        CV.notify_all();
        kill(Thread.native_handle(), SIGINT);
    }
    Thread.join();
}

TGaiTask::result_t TGaiTask::ProcessNext(std::string &host) {
    addrinfo *info = nullptr;
    addrinfo *node = nullptr;
    void *ptr = nullptr;

    int errorCode = getaddrinfo(host.c_str(), nullptr, &Hints, &info);
    result_t res = "Host: " + host + "\n";
    if (errorCode != 0) {
        res += "Getaddrinfo() failed. ";
        res += gai_strerror(errorCode);
        res += "\n";
        return res;
    }

    for (node = info; node; node = node->ai_next) {
        if (node->ai_family == AF_INET) {
            ptr = &(reinterpret_cast<sockaddr_in *>(node->ai_addr)->sin_addr);
        } else if (node->ai_family == AF_INET6) {
            ptr = &(reinterpret_cast<sockaddr_in6 *>(node->ai_addr)->sin6_addr);
        } else {
            continue;
        }
        inet_ntop(node->ai_family, ptr, AddrStr, IP_NODE_MAX_SIZE);
        res += AddrStr;
        res += "\n";
    }
    freeaddrinfo(info);
    return res;
}
