#include <iostream>
#include "ouroboros/http/io_context.hpp"
#include "ouroboros/http/server.hpp"

int main() {
    using namespace ouroboros::http;

    try {
        io_context ctx(4096);
        server srv(ctx, 8080);

        srv.start(); // Listen & Accept開始
        ctx.run();   // イベントループ突入

    } catch (const std::exception &e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}