#include "ConfigLoader.h"
#include <fstream>
using namespace std::literals;

std::expected<void, std::string>
ConfigLoader::Load(std::string_view filename) {
    std::ifstream file(std::string(filename).c_str());
    if (!file.is_open()) {
        return std::unexpected(std::format("无法打开文件: {}", filename));
    }
    config_.clear();
    std::string line;
    while (std::getline(file, line)) {
        ParseLine(line);
    }
    return {}; 
}

bool ConfigLoader::Contains(std::string_view key) const noexcept {
    return config_.contains(std::string(key));
}

std::expected<std::string, std::string>
ConfigLoader::GetString(std::string_view key) const noexcept {
    auto it = config_.find(std::string(key));
    if (it == config_.end()) {
        return std::unexpected(std::format("键不存在: {}", key));
    }
    return it->second;
}

std::expected<int, std::string>
ConfigLoader::GetInt(std::string_view key) const noexcept {
    auto value = GetString(key);
    if (!value) {
        return std::unexpected(value.error());
    }

    int result;
    auto [ptr, ec] = std::from_chars(
        value->data(), value->data() + value->size(), result
    );

    if (ec != std::errc{} || ptr != value->data() + value->size()) {
        return std::unexpected(std::format("无法转换为整数: {}", *value));
    }
    return result;
}

// 获取浮点数配置
std::expected<double, std::string>
ConfigLoader::GetDouble(std::string_view key) const noexcept {
    auto value = GetString(key);
    if (!value) {
        return std::unexpected(value.error());
    }

    double result;
    auto [ptr, ec] = std::from_chars(
        value->data(), value->data() + value->size(), result
    );

    if (ec != std::errc{} || ptr != value->data() + value->size()) {
        return std::unexpected(std::format("无法转换为浮点数: {}", *value));
    }
    return result;
}

// 获取布尔值配置（支持 true/false/1/0）
std::expected<bool, std::string>
ConfigLoader::GetBool(std::string_view key) const noexcept {
    auto value = GetString(key);
    if (!value) {
        return std::unexpected(value.error());
    }

    std::string_view sv = *value;
    sv = Trim(sv);
    if (sv == "true"sv || sv == "1"sv) return true;
    if (sv == "false"sv || sv == "0"sv) return false;
    return std::unexpected(std::format("无效的布尔值: {}", sv));
}

std::string_view ConfigLoader::Trim(std::string_view str) noexcept {
    auto start = str.find_first_not_of(" \t");
    if (start == std::string_view::npos) return "";
    auto end = str.find_last_not_of(" \t");
    return str.substr(start, end - start + 1);
}

// 解析单行配置（使用 string_view 避免拷贝）
void ConfigLoader::ParseLine(std::string_view line) {
    line = Trim(line);
    if (line.empty() || line.starts_with('#')) return;

    size_t pos = line.find('=');
    if (pos != std::string_view::npos) {
        auto key = Trim(line.substr(0, pos));
        auto value = Trim(line.substr(pos + 1));
        if (!key.empty()) {
            config_.emplace(key, value);
        }
    }
}