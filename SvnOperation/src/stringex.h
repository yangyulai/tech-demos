#pragma once
#include <string>
#include <windows.h>
#include <vector>
#include <filesystem>

inline std::wstring stringToWstring(const std::string& str) {
	int size_needed = MultiByteToWideChar(CP_ACP, 0, str.data(), (int)str.size(), nullptr, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_ACP, 0, str.data(), (int)str.size(), wstrTo.data(), size_needed);
	return wstrTo;
}
// Convert wstring to string
inline std::string wstringToString(const std::wstring& wstr) {
	int size_needed = WideCharToMultiByte(CP_ACP, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_ACP, 0, wstr.data(), (int)wstr.size(), strTo.data(), size_needed, nullptr, nullptr);
	return strTo;
}
// Convert wstring to string
inline std::string utf8ToString(const std::string& str) {
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), wstrTo.data(), size_needed);

	size_needed = WideCharToMultiByte(CP_ACP, 0, wstrTo.data(), (int)wstrTo.size(), nullptr, 0, nullptr, nullptr);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_ACP, 0, wstrTo.data(), (int)wstrTo.size(), strTo.data(), size_needed, nullptr, nullptr);
	return strTo;
}

inline std::wstring ansi_to_wstring(const std::string& s)
{
	// ACP = ANSI Code Page，告诉他字符串里的是当前区域设置指定的编码（在中国区，ANSI 就是 GBK 了）
	int len = MultiByteToWideChar(CP_ACP, 0,
		s.c_str(), static_cast<int>(s.size()),
		nullptr, 0);
	std::wstring ws(len, 0);
	MultiByteToWideChar(CP_ACP, 0,
		s.c_str(), static_cast<int>(s.size()),
		ws.data(), static_cast<int>(ws.size()));
	return ws;
}

inline std::string wstring_to_ansi(const std::wstring& ws)
{
	int len = WideCharToMultiByte(CP_ACP, 0,
		ws.c_str(), static_cast<int>(ws.size()),
		nullptr, 0,
		nullptr, nullptr);
	std::string s(len, 0);
	WideCharToMultiByte(CP_ACP, 0,
		ws.c_str(), static_cast<int>(ws.size()),
		s.data(), static_cast<int>(s.size()),
		nullptr, nullptr);
	return s;
}

inline std::wstring utf8_to_wstring(const std::string& s){
	int len = MultiByteToWideChar(CP_UTF8, 0,
		s.c_str(), static_cast<int>(s.size()),
		nullptr, 0);
	std::wstring ws(len, 0);
	MultiByteToWideChar(CP_UTF8, 0,
		s.c_str(), static_cast<int>(s.size()),
		ws.data(), static_cast<int>(ws.size()));
	return ws;
}

inline std::string wstring_to_utf8(const std::wstring& ws)
{
	int len = WideCharToMultiByte(CP_UTF8, 0,
		ws.c_str(), static_cast<int>(ws.size()),
		nullptr, 0,
		nullptr, nullptr);
	std::string s(len, 0);
	WideCharToMultiByte(CP_UTF8, 0,
		ws.c_str(), static_cast<int>(ws.size()),
		s.data(), static_cast<int>(s.size()),
		nullptr, nullptr);
	return s;
}
namespace str
{
	std::vector<std::wstring> getInputFile();
	std::vector<std::filesystem::path> getInputPath();
	std::vector<std::filesystem::path> cin();
}
