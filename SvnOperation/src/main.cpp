#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <string>
#include <print>
#include <boost/process/v1/io.hpp>
#include <boost/process/v1/child.hpp>
#include <boost/process/v1/search_path.hpp>
#include <boost/process/v1/args.hpp>
#include "BeastHttp.hpp"
#include "XL2JSON.h"

namespace bp = boost::process::v1;

int SvnCommit() {
    try {
        std::vector<std::string> args = {
            "commit",
            "./", 
            "-m", "修复了XXX功能的bug"
        };

        // 2. 启动子进程
        bp::ipstream out, err;
        bp::child c(
            bp::search_path("svn"), 
            bp::args(args),
            bp::std_out > out,
            bp::std_err > err
        );

        // 3. 读取并打印输出
        std::string line;
        while (std::getline(out, line)) {
            std::cout << "[OUT] " << line << '\n';
        }
        while (std::getline(err, line)) {
            std::cerr << "[ERR] " << line << '\n';
        }

        c.wait();
        int ec = c.exit_code();
        if (ec != 0) {
            return ec;
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "执行过程中出现异常: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
int main()
{
    boost::asio::io_context ioc;
    HttpServer server(ioc, { asio::ip::make_address("0.0.0.0"), 8080 });
    server.add_handler(http::verb::get, "/to_json", [](boost::urls::url_view u, unsigned version) {
        auto it = u.encoded_params().find("name");
        if (it != u.encoded_params().end())
        {
            XL2JSON etl("config.ini", L"ToJsonConfig.lua");
            etl.show_tips();
            try {
                etl.set_paths(it->value);
                etl.execute();
            }
            catch (const std::exception& e) {
                std::print("处理文件时出错:{}\n", e.what());
            }
            SvnCommit();
        }
        http::response<http::string_body> res{ http::status::ok, version };
        res.set(http::field::content_type, "text/plain; charset=utf-8");
        res.body() = "导入数据库成功\n";
        res.prepare_payload();
        return res;
        });

    server.run();
    ioc.run();
    return 0;
}