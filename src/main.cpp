#include "ouroboros/http.hpp"
#include <iostream>
#include <vector>
#include <stdexcept>

// A simple, standalone handler function for the root path.
void HomeHandler(const ouroboros::http::request& req, ouroboros::http::response& res) {
    res.set_body("Welcome to the Ouroboros HTTP server!");
    res.set_header("Content-Type", "text/plain");
}

// A controller-style class to group related handlers.
class ApiController {
public:
    void Login(const ouroboros::http::request& req, ouroboros::http::response& res) {
        // In a real application, you would validate credentials here.
        res.set_status_code(200);
        res.set_body("{\"status\": \"ok\", \"message\": \"Logged in successfully\"}");
        res.set_header("Content-Type", "application/json");
    }
};

int main() {
    using namespace ouroboros::http;

    try {
        io_context context;
        auto server_or_error = server::create(context, 8080);
        if (!server_or_error) {
            std::cerr << "Server creation failed: " << server_or_error.error().message() << std::endl;
            return 1;
        }

        auto& svr = server_or_error.value();

        // --- New Framework Usage ---
        ApiController api;

        // Define the routing table using the route_entry struct.
        std::vector<route_entry> routes = {
            { method::GET,  "/",       HomeHandler },
            { method::POST, "/login",  bind_member(&ApiController::Login, &api) }
        };

        // Load the routes into the server instance.
        svr.load_routes(routes);
        std::cout << "Routing table loaded." << std::endl;
        // -------------------------

        auto start_or_error = svr.start();
        if(!start_or_error) {
             std::cerr << "Server start failed: " << start_or_error.error().message() << std::endl;
            return 1;
        }

        context.run();

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}