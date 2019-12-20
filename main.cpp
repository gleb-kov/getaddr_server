#include <iostream>

#include "server.h"
#include "gai_client.h"

int main() {
    try {
        TIOWorker io_context;
        TServer<TGaiClient> server(io_context, htonl(INADDR_ANY), htons(58239));
        io_context.Exec(1000);
    } catch (std::exception const &e) {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "UNKNOWN EXCEPTION." << std::endl;
    }
    return 0;
}
