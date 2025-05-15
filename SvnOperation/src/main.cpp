#define WIN32_LEAN_AND_MEAN
#include <string>
#include <boost/process/v1/io.hpp>
#include <boost/process/v1/child.hpp>
#include <boost/process/v1/search_path.hpp>
#include <boost/process/v1/args.hpp>
#include <boost/process/v1/start_dir.hpp>

#include "AppConfig.h"
#include "BeastHttp.hpp"
#include "stringex.h"
#include "XL2JSON.h"

// 获取当前本地时间的字符串 "[YYYY-MM-DD HH:MM:SS] "
static std::string get_timestamp()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    auto tt = system_clock::to_time_t(now);
    std::tm tm;
    localtime_s(&tm, &tt);  // Windows 下安全版本

    // 直接用 std::format 格式化 tm 结构
    return std::format("[{:04}-{:02}-{:02} {:02}:{:02}:{:02}] ",
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec
    );
}

void log_line(const std::string& tag, const std::string& msg) {
    // 1. 先拼时间戳
    app::request += get_timestamp();

    // 2. 再拼 tag + msg，并做对齐
    //    时间部分已经包含尾部空格了，所以这里只对 [tag] 和 msg 对齐
    app::request += std::format("{:<14} {}\r\n",
        "[" + tag + "]",
        msg
    );
}

void print_border(const std::string& title) {
    app::request += std::format("┌{:─^40}┐\r\n", "【 " + title + " 】");
    app::request += std::format("└{:─^40}┘\r\n\r\n", "");
}

namespace bp = boost::process::v1;
// 运行一个 svn 子命令，打印 stdout/stderr，并返回子进程退出码
int RunSvnCommand(const std::vector<std::string>& args,
    std::string describe,
    const std::string& working_dir = "./")
{
    try {
        bp::ipstream out, err;
        bp::child c(
            bp::search_path("svn"),
            bp::args(args),
            bp::start_dir(working_dir),
            bp::std_out > out,
            bp::std_err > err
        );

        std::string line;
        while (std::getline(out, line)) {
			log_line(describe, line);
            app::request += '\n';
        }
        while (std::getline(err, line)) {
            log_line("[svn err] ", line);
            app::request += "[svn err] ";
        }

        c.wait();
        return c.exit_code();
    }
    catch (const std::exception& ex) {
        log_line("执行 SVN 命令异常: ", ex.what());
        return -1;
    }
}

// 提交改动
int SvnCommit(const std::string& message,
    const std::string& path = "./")
{
    std::string ms = utf8ToString(message);
    std::vector<std::string> args = {
        "commit",
        path,
        "-m", ms
    };
    return RunSvnCommand(args,"服务器提交", path);
}

// 更新工作拷贝
int SvnUpdate(const std::string& path = "./")
{
    std::vector<std::string> args = {
        "update",
        path
    };
    return RunSvnCommand(args, "服务器更新", path);
}


int main()
{
    boost::asio::io_context ioc;
    HttpServer server(ioc, { asio::ip::make_address("0.0.0.0"), 8080 });
    server.add_handler(http::verb::get, "/to_json", [](boost::urls::url_view u, unsigned version) {
        http::response<http::string_body> res{ http::status::ok, version };
        do
        {
			//std::cout << "begin1\n";
			app::request.clear();
            auto it = u.encoded_params().find("name");
            if (it != u.encoded_params().end())
            {
                //std::cout << "begin4\n";
				std::string table_name = std::string(it->value);
                //std::cout << "begin5\n";
                print_border("开始处理");
                //std::cout << "begin6\n";
                log_line("表", table_name);
                //std::cout << "begin7\n";
                // 先更新
                //std::cout << "begin3\n";
                if (int code = SvnUpdate(utf8ToString(app_config::excel_path)); code != 0) {
                    app::request += std::format("SVN 更新失败，退出码：{}", code);
                    std::cout << "begin4\n";
                    break;
                }
				//std::cout << "begin2\n";
                try {
                    XL2JSON etl;
                    etl.set_paths(app_config::excel_path + std::string(table_name));
                    etl.execute();
                }
                catch (const std::exception& e) {
                    app::request += std::format("【失败】:{}\n", e.what());
                    break;
                }
                // 然后提交
                if (int code = SvnCommit("游戏配置提交", utf8ToString(app_config::json_path)); code != 0) {
                    app::request += std::format("SVN 提交失败，退出码：{}", code);
				}
				else {
                    app::request += "\r\n";
                    print_border("处理成功");
				}
                break;
            }else
            {
                app::request += "缺少参数【name=表名】\n";
				//std::cout << "fail\n";
                break;
            }
        } while (false);
        res.set(http::field::content_type, "text/plain; charset=utf-8");
        res.body() = app::request;
        res.prepare_payload();
        return res;
        });

    server.run();
    ioc.run();
    return 0;
}