#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <format>
#include <unordered_map>

class ConfigLoader {
public:
    [[nodiscard]] std::expected<void, std::string>
        Load(std::string_view filename);

    [[nodiscard]] bool Contains(std::string_view key) const noexcept;

    [[nodiscard]] std::expected<std::string, std::string>
        GetString(std::string_view key) const noexcept;

    [[nodiscard]] std::expected<int, std::string>
        GetInt(std::string_view key) const noexcept;

    [[nodiscard]] std::expected<double, std::string>
        GetDouble(std::string_view key) const noexcept;

    [[nodiscard]] std::expected<bool, std::string>
        GetBool(std::string_view key) const noexcept;

private:
    std::unordered_map<std::string, std::string> config_;

    static std::string_view Trim(std::string_view str) noexcept;

    void ParseLine(std::string_view line);
};

