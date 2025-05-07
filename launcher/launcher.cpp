
#include "BeastHttp.hpp"
#include <boost/asio.hpp>
#include <boost/uuid.hpp>
#include <iostream>

int main() {
    boost::asio::io_context ioc;
    HttpServer server(ioc, { asio::ip::make_address("0.0.0.0"), 8080 });
    // Register handlers for different routes
    server.add_handler(http::verb::get, "/ping", [](http::request<http::string_body>&& req) {
        http::response<http::string_body> res{ http::status::ok, req.version() };
        res.set(http::field::content_type, "text/plain");
        res.body() = "pong";
        res.prepare_payload();
        return res;
        });

    server.add_handler(http::verb::post, "/echo", [](http::request<http::string_body>&& req) {
        http::response<http::string_body> res{ http::status::ok, req.version() };
        res.set(http::field::content_type, "application/json");
        res.body() = req.body();   // echo request body
        res.prepare_payload();
        return res;
        });

    server.run();
    ioc.run();
    return 0;
}



// launcher.cpp
//#include <boost/beast.hpp>
//#include <boost/asio.hpp>
//#include <boost/process.hpp>
//#include <nlohmann/json.hpp>
//#include <iostream>
//#include <string>
//#include <boost/process/v1/child.hpp>
//#include <boost/process/v1/exe.hpp>
//#include <boost/process/v1/args.hpp>
//#include <boost/process/v1/io.hpp>
//#include <boost/algorithm/string/predicate.hpp>
//namespace beast   = boost::beast;     // from <boost/beast.hpp>
//namespace http    = beast::http;      // from <boost/beast/http.hpp>
//namespace asio    = boost::asio;      // from <boost/asio.hpp>
//namespace bp      = boost::process::v1;   // from <boost/process.hpp>
//using json        = nlohmann::json;
//
//int main(int argc, char* argv[]) {
//    try {
//        // 1. 构造 I/O 上下文
//        asio::io_context ioc;
//
//        // 2. 解析地址和端口
//        const std::string host = "175.24.114.29";
//        const std::string port = "19880";
//        const std::string target = "/token.php";
//        const int version = 11; // HTTP/1.1
//
//        // 3. 建立 TCP 连接
//        asio::ip::tcp::resolver resolver{ioc};
//        auto const results = resolver.resolve(host, port);
//        beast::tcp_stream stream{ioc};
//        stream.connect(results);
//
//        // 4. 发送 HTTP GET
//        http::request<http::string_body> req{http::verb::get, target, version};
//        req.set(http::field::host, host);
//        req.set(http::field::user_agent, "MyLauncher/1.0");
//        http::write(stream, req);
//
//        // 5. 读取响应
//        beast::flat_buffer buffer;
//        http::response<http::string_body> res;
//        http::read(stream, buffer, res);
//
//        // 6. 关闭连接
//        beast::error_code ec;
//        stream.socket().shutdown(asio::ip::tcp::socket::shutdown_both, ec);
//        // (忽略 shutdown 错误)
//
//        if (res.result() != http::status::ok) {
//            std::cerr << "Config server returned " << res.result_int() << "\n";
//            return 1;
//        }
//        auto args = res.body();
//
//
//        // 9. 启动子进程
//        bp::child server_proc(
//            bp::exe = R"(F:\SkillRoad\SVR\gameserver.exe)",
//            bp::args = args
//        );
//
//        std::cout << "Launched server (pid=" << server_proc.id() << ")\n";
//
//        // 10. 等待服务器退出（也可以不等待，直接返回）
//        server_proc.wait();
//        return server_proc.exit_code();
//    }
//    catch (std::exception& e) {
//        std::cerr << "Error: " << e.what() << "\n";
//        return 1;
//    }
//}
