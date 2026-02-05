# Ouroboros HTTP Framework Requirements Definition

## 1. Overview

This document defines the requirements for a new HTTP server framework component within the Ouroboros project. The goal is to create a robust, readable, and maintainable API for handling HTTP requests, based on the "Centralized Routing" model. This approach moves away from the inline lambda style of other libraries to improve clarity and maintainability.

## 2. Core Design Principle: Centralized Routing

The framework will adopt a "Centralized Routing" model. This pattern separates routing definitions from handler logic. Developers will define a list of routes (a routing table) and load it into the server instance at once.

**Key Benefits:**
- **Improved Readability:** Routing logic is centralized and declarative, making it easier to understand the server's endpoints.
- **Stronger Typing:** Avoids ambiguities in overload resolution common with complex inline lambdas.
- **Maintainability:** Handler logic is decoupled from the routing mechanism, simplifying testing and modification.

## 3. Technical Specifications

### 3.1. Language Standard
- C++17 or newer.

### 3.2. Core Components

#### `Method` Enum
An `enum class` will define supported HTTP methods to ensure type safety. This will be placed in a new header `include/ouroboros/http/method.hpp`.
```cpp
enum class Method {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS
};
```

#### `Request` and `Response` Objects
- Handler functions will receive `Request` and `Response` objects.
- `Request`: Provides `const` access to request headers, body, path, and method.
- `Response`: Allows setting the response body, headers, and status code.
- These will be defined as placeholder classes initially in a new header, e.g., `include/ouroboros/http/request_response.hpp`.

```cpp
// A simplified representation
class Request {
    // Provides access to method, path, headers, body, etc.
};

class Response {
public:
    void SetBody(const std::string& body);
    void SetHeader(const std::string& name, const std::string& value);
    void SetStatusCode(int code);
};
```

#### `HandlerFunc` Type
The handler function signature will be defined as a `std::function`.
```cpp
using HandlerFunc = std::function<void(const Request&, Response&)>;
```

#### `RouteEntry` Struct
A struct will represent a single entry in the routing table.
```cpp
struct RouteEntry {
    Method method;
    std::string path;
    HandlerFunc handler;
};
```

### 3.3. `Server` Class API

The existing `ouroboros::http::Server` class will be augmented with the following method:

#### `LoadRoutes` Method
- This method accepts a vector of `RouteEntry` objects to configure the server's routing table.
- The internal storage will be a structure optimized for efficient lookups, such as a map keyed by method and path.

```cpp
// In class ouroboros::http::Server
class Server {
public:
    // ... existing methods ...

    void LoadRoutes(const std::vector<RouteEntry>& routes);

private:
    // Example internal routing table:
    // std::map<Method, std::map<std::string, HandlerFunc>> routes_;
};
```

### 3.4. Handler Binding

#### `BindMember` Helper
A helper function will be provided to facilitate binding class member functions to the `HandlerFunc` type, improving ergonomics. This can be provided in a utility header like `include/ouroboros/http/binder.hpp`.

```cpp
template<typename F, typename C>
auto BindMember(F func, C* instance) {
    return [=](const Request& req, Response& res) {
        (instance->*func)(req, res);
    };
}
```

## 4. Integration and Usage Example

The framework should enable the following user code, which will serve as the basis for a new `main.cpp`.

```cpp
#include "ouroboros/http.hpp" // An umbrella header will be created

// Handler function
void HomeHandler(const Request& req, Response& res) {
    res.SetBody("Welcome");
}

class ApiController {
public:
    void Login(const Request& req, Response& res) {
        res.SetBody("Logged in");
    }
};

int main() {
    using namespace ouroboros::http;

    ApiController api;
    Server svr;

    // Define the routing table
    std::vector<RouteEntry> routes = {
        { Method::GET,  "/",       HomeHandler },
        { Method::POST, "/login",  BindMember(&ApiController::Login, &api) }
    };

    // Load routes into the server
    svr.LoadRoutes(routes);
    
    // Start the server
    svr.Listen("localhost", 8080);
    return 0;
}
```

## 5. Implementation Plan

1.  Create necessary headers for `Method`, `Request`/`Response`, `RouteEntry`, and `BindMember`.
2.  Modify `ouroboros::http::Server` in `server.hpp` to include the `LoadRoutes` method and routing table.
3.  Adapt the request handling logic in `http_session.cpp` to perform lookups in the new routing table and execute handlers.
4.  Create an umbrella header `ouroboros/http.hpp` for convenient inclusion.
5.  Replace the content of `src/main.cpp` with the usage example to test the new framework.
6.  Update `CMakeLists.txt` to ensure all new files are included in the build.
