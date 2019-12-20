#include "gai_client.h"

TGaiClient::TGaiClient() {
    callback = [this](TIOTask *const self, uint32_t events) noexcept {
        CallbackWrapper(self, events);
    };
}

void TGaiClient::CallbackWrapper(TIOTask *const self, uint32_t events) noexcept {
    if ((events & EPOLLOUT) &&
        (QueryProcessor.HaveResult() || !ResultSuffix.empty()))
    {
        if (ResultSuffix.size() < 1024) {  // just set custom limit
            try {
                ResultSuffix += QueryProcessor.GetResult();
            } catch (...) {
                self->Close();
            }
        }

        int code = self->Write(ResultSuffix.c_str(), ResultSuffix.size());
        if (code > 0) {
            ResultSuffix.erase(ResultSuffix.begin(), ResultSuffix.begin() + code);
        } else {
            self->Close();
        }
    } else if ((events & EPOLLIN) && QueryProcessor.HaveFreeSpace()) {
        int code = self->Read(Buffer, TGetaddrinfoTask::QUERY_MAX_LENGTH);
        if (code > 0) {
            try {
                QueryProcessor.SetTask(Buffer, code);
            } catch(...) {
                self->Close();
            }
        } else {
            self->Close();
        }
    }
    uint32_t actions = 0;
    if (QueryProcessor.HaveFreeSpace()) {
        actions |= EPOLLIN;
    }
    if (QueryProcessor.HaveUnprocessed() ||
        QueryProcessor.HaveResult() ||
        !ResultSuffix.empty()) {
        actions |= EPOLLOUT;
    }
    self->Reconfigure(actions);
}
