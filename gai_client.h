#ifndef GETADDR_SERVER_GAI_CLIENT_H
#define GETADDR_SERVER_GAI_CLIENT_H

#include <functional>
#include <sys/epoll.h>

#include "iotask.h"
#include "gai_task.h"

class TGaiClient {
public:
    TGaiClient();

    ~TGaiClient() = default;

    TGaiClient(TGaiClient const &) = delete;

    TGaiClient(TGaiClient &&) = delete;

    TGaiClient &operator=(TGaiClient const &) = delete;

    TGaiClient &operator=(TGaiClient &&) = delete;

private:
    void CallbackWrapper(TIOTask *self, uint32_t events) noexcept;

public:
    TIOTask::callback_t callback;

private:
    char Buffer[TGetaddrinfoTask::QUERY_MAX_LENGTH] = {0};
    std::string ResultSuffix;
    TGetaddrinfoTask QueryProcessor;
};

#endif //GETADDR_SERVER_GAI_CLIENT_H