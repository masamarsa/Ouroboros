#include "ouroboros/http/http_session.hpp"
#include "ouroboros/http/server.hpp"
#include <iostream>
#include <cstring>
#include <linux/io_uring.h>
#include <cerrno>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string_view>

namespace ouroboros::http
{
    constexpr size_t BUFFER_SIZE = 8192;

    http_session::http_session(server& svr, io_context &ctx, unique_socket socket)
        : server_(svr), ctx_(ctx), socket_(std::move(socket)), buffer_(BUFFER_SIZE) {}

    http_session::~http_session() {
        if (socket_.native_handle() != -1) {
             std::cout << "Session closing. FD: " << socket_.native_handle() << std::endl;
        } else {
             std::cout << "Session closed." << std::endl;
        }
    }

    void http_session::start() {
        submit_recv();
    }

    void http_session::handle_request() {
        std::string_view received_data(buffer_.data(), buffer_.size());

        // Basic HTTP/1.x request line parsing: "METHOD /path HTTP/1.x\r\n"
        size_t method_end = received_data.find(' ');
        if (method_end == std::string_view::npos) {
            submit_send("HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
            keep_alive_ = false;
            return;
        }
        size_t path_end = received_data.find(' ', method_end + 1);
        if (path_end == std::string_view::npos) {
            submit_send("HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n");
            keep_alive_ = false;
            return;
        }

        std::string method_str(received_data.substr(0, method_end));
        std::string path_str(received_data.substr(method_end + 1, path_end - (method_end + 1)));

        method method_enum;
        if (method_str == "GET") method_enum = method::GET;
        else if (method_str == "POST") method_enum = method::POST;
        else {
            submit_send("HTTP/1.1 501 Not Implemented\r\nConnection: close\r\n\r\n");
            keep_alive_ = false;
            return;
        }

        request req;
        req.method = method_enum;
        req.path = path_str;
        // A full parser would extract headers and body here.

        auto handler_opt = server_.find_handler(req.method, req.path);

        response res;
        if (handler_opt) {
            try {
                (*handler_opt)(req, res);
            } catch (const std::exception& e) {
                std::cerr << "Handler exception: " << e.what() << std::endl;
                res = response();
                res.set_status_code(500);
                res.set_body("Internal Server Error");
            }
        } else {
            res.set_status_code(404);
            res.set_body("Not Found");
        }
        
        const std::string_view conn_hdr("connection:");
        auto it = std::search(received_data.begin(), received_data.end(), conn_hdr.begin(), conn_hdr.end(),
            [](char a, char b){ return std::tolower(static_cast<unsigned char>(a)) == static_cast<unsigned char>(b); });
        
        keep_alive_ = true; // Default for HTTP/1.1
        if (it != received_data.end()) {
            std::string_view conn_val(it + conn_hdr.length());
            conn_val = conn_val.substr(conn_val.find_first_not_of(" \t"));
            conn_val = conn_val.substr(0, conn_val.find("\r\n"));
            if (conn_val.length() >= 5 && std::equal(conn_val.begin(), conn_val.begin() + 5, "close", 
                [](char a, char b){ return std::tolower(static_cast<unsigned char>(a)) == static_cast<unsigned char>(b); })) {
                keep_alive_ = false;
            }
        }

        std::stringstream ss;
        ss << "HTTP/1.1 " << res.status_code() << " ";
        switch(res.status_code()) {
            case 200: ss << "OK"; break;
            case 400: ss << "Bad Request"; break;
            case 404: ss << "Not Found"; break;
            case 500: ss << "Internal Server Error"; break;
            case 501: ss << "Not Implemented"; break;
            default: ss << "OK"; break;
        }
        ss << "\r\n";
        ss << "Content-Length: " << res.body().length() << "\r\n";
        ss << "Connection: " << (keep_alive_ ? "keep-alive" : "close") << "\r\n";
        for(const auto& [key, val] : res.headers()) {
            ss << key << ": " << val << "\r\n";
        }
        ss << "\r\n";
        ss << res.body();
        
        static thread_local std::string response_buffer;
        response_buffer = ss.str();
        submit_send(response_buffer);
    }

    void http_session::submit_recv() {
        pending_ops_++;
        current_state_ = state::reading;

        auto *sqe = ctx_.get_sqe();
        if (!sqe) {
            pending_ops_--;
            socket_ = unique_socket();
            if (pending_ops_ == 0) delete this;
            return;
        }

        sqe->opcode = IORING_OP_RECV;
        sqe->fd = socket_.native_handle();
        sqe->addr = (uint64_t)buffer_.data();
        sqe->len = buffer_.capacity();
        sqe->flags = 0;
        sqe->user_data = (uint64_t)this;

        ctx_.submit();
    }

    void http_session::submit_send(std::string_view data) {
        pending_ops_++;
        current_state_ = state::writing;

        auto *sqe = ctx_.get_sqe();
        if (!sqe) {
            pending_ops_--;
            socket_ = unique_socket();
            if (pending_ops_ == 0) delete this;
            return;
        }

        sqe->opcode = IORING_OP_SEND;
        sqe->fd = socket_.native_handle();
        sqe->addr = (uint64_t)data.data();
        sqe->len = static_cast<uint32_t>(data.size());
        sqe->flags = 0;
        sqe->user_data = (uint64_t)this;

        ctx_.submit();
    }

    void http_session::handle_read(int result, uint32_t flags) {
        if (result <= 0) {
            socket_ = unique_socket();
            return;
        }
        
        std::cout << "Received " << result << " bytes." << std::endl;
        buffer_.resize(result);

        handle_request();
    }

    void http_session::handle_write(int result) {
        if (result < 0) {
            std::cerr << "Send failed with error: " << -result << std::endl;
            socket_ = unique_socket();
        } else {
            if (keep_alive_) {
                buffer_.assign(buffer_.capacity(), 0);
                buffer_.resize(0);
                submit_recv();
            } else {
                socket_ = unique_socket();
            }
        }
    }

    void http_session::complete(int result, uint32_t flags) {
        pending_ops_--;

        if (current_state_ == state::reading) {
            handle_read(result, flags);
        } else if (current_state_ == state::writing) {
            handle_write(result);
        }

        if (pending_ops_ == 0 && !socket_) {
            delete this;
        }
    }
}
