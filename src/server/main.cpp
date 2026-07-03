#include <Server.hpp>

int main() {
    auto& srv = Server::get();

    return srv.init();
}