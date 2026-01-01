# Ouroboros 🐍

**A Native, High-Performance C++23 Web Server Library for Linux.**

> "The snake that eats its own tail." — A symbol of infinity and wholeness.

Ouroboros is a modern, header-only (or static) C++ web server library designed to extract the maximum performance from the Linux kernel. By leveraging `io_uring` and adhering strictly to C++ standard library philosophies, it eliminates the overhead found in traditional frameworks and other languages.

## ⚡ Philosophy

* **Native to the Core:** Built exclusively for Linux (Kernel 5.10+) using `io_uring` for fully asynchronous I/O. No `epoll`, no legacy wrappers.
* **STL Harmony:** We believe the library should feel like an extension of the language. All identifiers use `snake_case` to blend seamlessly with the C++ Standard Library.
* **Zero Overhead:** Detailed focus on memory efficiency. We aim for **zero heap allocations** (new/malloc) during request processing.
* **Performance First:** Targeting throughput and latency equivalent to or exceeding ASP.NET Core (Kestrel) and Rust (Actix-web).



## 🚀 Key Features

* **Modern C++ Standards:** Written in C++20/23, utilizing Concepts and `std::expected` for robust error handling.
* **io_uring Architecture:** Uses the raw `io_uring` interface (no `liburing` overhead) with `IO_URING_OP_PROVIDE_BUFFERS` for kernel-managed buffering.
* **Thread-per-Core Model:** Implements a Shared-Nothing architecture using `SO_REUSEPORT`. No mutex contention in user-land.
* **Zero-Copy Routing:** Request parsing operates entirely on `std::string_view` mapped to the ring buffer. No strings are constructed during parsing.

## 🛠 Usage

Ouroboros provides a clean, STL-like API.

```cpp
#include "ouroboros/http/server.hpp"

int main() {
    using namespace ouroboros;

    // Initialize the IO context (Event Loop)
    http::io_context ctx;
    
    // Create the server instance
    http::server server(ctx);

    // Register a route: STL-like interface
    server.get("/api/status", [](const auto& req) -> http::response {
        return {200, "application/json", R"({"status":"ok"})"};
    });

    // Zero-copy path parameters
    server.get("/users/:id", [](const auto& req) {
        // 'id' is a std::string_view pointing directly to the receive buffer
        auto user_id = req.params["id"]; 
        return lookup_user(user_id);
    });

    // Start listening and run the blocking event loop
    server.listen(8080);
    ctx.run(); 
}

```



## 🏗 Architecture

### Namespace Structure

* **`ouroboros::http`**: The core. Contains the low-latency HTTP server, client, WebSocket, and `io_uring` wrappers.
* **`ouroboros::www` (Planned)**: A future application layer featuring a React/Vue-like component system for high-speed Server-Side Rendering (SSR).

### Internals

* **`unique_socket`**: A RAII wrapper for file descriptors, ensuring strictly managed lifecycles.
* **State Machine Parser**: A pointer-based parser that never allocates memory.
* **Error Handling**: Uses `std::expected` for control flow (404, parsing errors) and exceptions only for fatal/recoverable errors.

## 📋 Requirements

* **OS**: Linux (Kernel 5.10 or higher required for full `io_uring` support).
* **Compiler**: GCC 11+ or Clang 14+ (Must support C++20/23).
* **Dependencies**: None (Standard Library & Linux Syscalls only). OpenSSL is optional.

## 🔮 Roadmap (The "www" Vision)

While the current focus is the `http` communication layer, our ultimate goal is to build `ouroboros::www`: a component-oriented framework that enables building complex Web Applications with the raw speed of C++. We aim to revolutionize Server-Side Rendering.

---

*Created by the Ouroboros Project Team.*

---

### Next Step for You

Would you like me to create the **`CMakeLists.txt`** file or a **`.clang-format`** file to match the `snake_case` and C++23 requirements for this repository?
