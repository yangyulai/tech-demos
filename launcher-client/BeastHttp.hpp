#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <string>
#include <thread>
#include <functional>
#include <unordered_map>
#include <boost/beast/version.hpp>
#include <mutex>
#include <optional>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

// Utility: parse a specific cookie from the Cookie header
inline std::string parse_cookie(const std::string& cookie_header, const std::string& key) {
    size_t start = cookie_header.find(key + "=");
    if (start == std::string::npos) return {};
    start += key.size() + 1;
    size_t end = cookie_header.find(';', start);
    return cookie_header.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

// Represents stored session data for a user
struct UserSession {
    std::string username;
    std::vector<std::string> roles;   // Permissions or roles
    std::string device_id;
};

// Thread-safe in-memory session store
class SessionStore {
public:
    void add(const std::string& sid, UserSession&& session) {
        std::lock_guard lock(mtx_);
        sessions_[sid] = std::move(session);
    }

    std::optional<UserSession> get(const std::string& sid) {
        std::lock_guard lock(mtx_);
        auto it = sessions_.find(sid);
        if (it == sessions_.end()) return std::nullopt;
        return it->second;
    }

    void remove(const std::string& sid) {
        std::lock_guard lock(mtx_);
        sessions_.erase(sid);
    }

private:
    std::unordered_map<std::string, UserSession> sessions_;
    std::mutex mtx_;
};

class HttpClient {
public:
    explicit HttpClient(asio::io_context& ioc)
        : ioc_(ioc) {
    }
    std::string get(const std::string& host,
        const std::string& port,
        const std::string& target,
        int version = 11) const
    {
        // Resolver and stream
        tcp::resolver resolver(ioc_);
        beast::tcp_stream stream(ioc_);

        auto const results = resolver.resolve(host, port);
        stream.connect(results);

        // Build the request
        http::request<http::empty_body> req{ http::verb::get, target, version };
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send request
        http::write(stream, req);

        // Buffer for response
        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        // Gracefully close
        beast::error_code ec;
        ec = stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        // Ignore not_connected errors
        if (ec && ec != beast::errc::not_connected)
            throw beast::system_error{ ec };

        return res.body();
    }

private:
    asio::io_context& ioc_;
};
using RequestHandler = std::function<http::response<http::string_body>(http::request<http::string_body>)>;

// Asynchronous HTTP server
class HttpServer {
public:

    HttpServer(asio::io_context& ioc,
        const tcp::endpoint& endpoint)
        : ioc_(ioc)
        , acceptor_(asio::make_strand(ioc))
    {
        beast::error_code ec;

        // Open and bind
        ec = acceptor_.open(endpoint.protocol(), ec);
        ec = acceptor_.set_option(asio::socket_base::reuse_address(true), ec);
        ec = acceptor_.bind(endpoint, ec);
        ec = acceptor_.listen(asio::socket_base::max_listen_connections, ec);
    }
    void add_handler(http::verb method, const std::string& target, RequestHandler handler) {
        routes_[{method, target}] = std::move(handler);
    }

    // Start accepting
    void run() {
        do_accept();
    }

private:
    struct RouteKey {
        http::verb method;
        std::string target;
        bool operator==(const RouteKey& o) const {
            return method == o.method && target == o.target;
        }
    };
    struct RouteHash {
        std::size_t operator()(const RouteKey& k) const noexcept {
            return std::hash<int>()(static_cast<int>(k.method)) ^ std::hash<std::string>()(k.target);
        }
    };

    asio::io_context& ioc_;
    tcp::acceptor acceptor_;
    std::unordered_map<RouteKey, RequestHandler, RouteHash> routes_;
    void do_accept() {
        acceptor_.async_accept(
            asio::make_strand(ioc_),
            beast::bind_front_handler(
                &HttpServer::on_accept,
                this));
    }

    void on_accept(beast::error_code ec, tcp::socket socket) {
        if (!ec) {
            std::thread{
                [this, sock = std::move(socket)]() mutable {
                    do_session(std::move(sock));
                }
            }.detach();
        }
        do_accept();
    }

    void do_session(tcp::socket socket) {
        beast::error_code ec;
        beast::flat_buffer buffer;

        http::request<http::string_body> req;
        http::read(socket, buffer, req, ec);
        if (ec == http::error::end_of_stream)
            return;
        if (ec)
            return;

        auto key = RouteKey{ req.method(), std::string(req.target()) };
        auto it = routes_.find(key);
        http::response<http::string_body> res;
        if (it != routes_.end()) {
            res = it->second(std::move(req));
        }
        else {
            res = http::response<http::string_body>{ http::status::not_found, req.version() };
            res.set(http::field::server, "BeastHttpServer");
            res.set(http::field::content_type, "text/plain");
            res.body() = "404 Not Found";
            res.prepare_payload();
        }

        http::write(socket, res, ec);
        ec = socket.shutdown(tcp::socket::shutdown_send, ec);
    }
};
