#pragma once
#include <string>
#define BO_LOCAL
#undef BO_LOCAL
namespace app_config {
    // 网络配置
    inline std::string json_suffix = ".json";
    inline std::string cpp_file_name = "JsonStruct.h";
#ifndef BO_LOCAL
    inline std::string json_path = "D:/ARPG7Cool/data/";
    inline std::string excel_path = "D:/数据库/";
    inline std::string ToJsonConfig = "D:/数据库/ToJsonConfig.lua";
#else
	inline std::string json_path = "E:/701/2D_701/SVR/origin/data/";
	inline std::string excel_path = "E:/701/2D_701/Client/数据库/";
	inline std::string ToJsonConfig = "E:/701/2D_701/Client/数据库/ToJsonConfig.lua";
#endif
    inline std::string request;

}
namespace app {
    inline std::string request;

}