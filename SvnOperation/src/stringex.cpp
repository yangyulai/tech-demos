#include "stringex.h"
#include <iostream>
#include <vector>
#include <print>
namespace str
{
	std::vector<std::wstring> getInputFile() {
		std::wstring input;
		std::getline(std::wcin, input);

		std::wistringstream wiss(input);
		std::wstring word;
		std::vector<std::wstring> words;

		while (wiss >> word) {
			words.push_back(word);
		}
		return words;
	}

	std::vector<std::filesystem::path> getInputPath() {

		std::wcout << L"请输入文件路径（拖拽一个或多个）：" << std::endl;

		std::wstring input;
		std::getline(std::wcin, input);

		std::wistringstream wiss(input);
		std::wstring word;
		std::vector<std::filesystem::path> paths;

		while (wiss >> word) {
			paths.emplace_back(word);
		}
		return paths;
	}
	std::vector<std::filesystem::path> cin()
	{
		std::print("请输入文件路径（拖拽一个或多个）：\n");

		std::string input;
		std::getline(std::cin, input);

		std::stringstream iss(input);
		std::string word;
		std::vector<std::filesystem::path> paths;

		while (iss >> word)
		{
			paths.emplace_back(word);
		}
		return paths;
	}
}
